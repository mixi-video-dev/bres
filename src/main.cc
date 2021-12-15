#include "bres+client_ext.h"
#include "bres+bm.hh"
//
int main(
    int argc,
    char *argv[]
)
{
    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);
    srand((unsigned) time(NULL) * getpid());

    bres::setAffinity(CPUAFFINITY_MAIN);

    auto window = bres::gl_init();
    bres::instance_t publish_instance;
    std::vector<bres::instance_t> pooled_instances(bres::INSTANCE_MAX);
    std::vector<std::thread> threads;
    bres::events_t ipc_events;
    std::vector<char> videoBuffer(WND_WIDTH_FHD * WND_HEIGHT_FHD * 4);

    // allocate 
    for(auto& inst : pooled_instances) {
        inst.lastTextureBuffer_ = std::vector<char>(WND_WIDTH * WND_HEIGHT * 4);
    }
    // start telnet server
    threads.push_back(std::thread(
        [&ipc_events]{
            bres::setAffinity(CPUAFFINITY_TELNET);
            bres::telnet_service(&ipc_events);
        }
    ));

#ifndef __BLACKMAGIC_OUTPUT__
    bres::publishSideAddChannel(publish_instance, "", 0);
    std::vector<void*> audio_args = {&pooled_instances, &publish_instance};
#else
    // halt until prepared peerConnection
    usleep(1000000*1);
    // initialize blackmagic devices
    bres::Device device(NULL);
    std::vector<void*> audio_args = {&pooled_instances, &device};

    // halt until blackmagic found devices
    usleep(1000000*3);

    // start pipeline process for blackmagic devices
    threads.push_back(std::thread(
        [&device]{
            bres::setAffinity(CPUAFFINITY_BMDEV);
            bres::bm_pipeline_service(&device);
        }
    ));
#endif
    bres::audioInit(&audio_args);

    struct timeval tm;
    unsigned long prev_usec = 0;
    unsigned counter = 1;
    //
    while (!glfwWindowShouldClose(window) &&
            glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
    {
        char* pframe = NULL;

        glfwPollEvents();
        gettimeofday(&tm, NULL);
        auto now = tm.tv_sec*(uint64_t)1000000+tm.tv_usec;

        glClearColor(0, 1, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // collect available frames.
        for(auto& inst : pooled_instances) {
            if (inst.instance_enum != bres::INSTANCE_MIN) {
                if (bres::pop_bin_event(&inst, bres::EVENT_TYPE_CURRENT_IMAGE, &pframe)) {
                    memcpy(inst.lastTextureBuffer_.data(), pframe, (WND_WIDTH*WND_HEIGHT*4));
                    bres::push_bin_event(&inst, bres::EVENT_TYPE_EMPTY_IMAGE, pframe);
                }
            }
        }
        //
        for(auto& inst : pooled_instances) {
            if (inst.isDraw) {
                bres::gl_movie_play(
                    &inst,
                    inst.lastTextureBuffer_.data(),
                    1.0f, 1.0f,
                    inst.posX,
                    inst.posY,
                    inst.width,
                    inst.height,
                    inst.scale,
                    inst.scale);
            }
        }
        //
        glfwSwapBuffers(window);
#ifndef __BLACKMAGIC_OUTPUT__
        if (publish_instance.capture_ && publish_instance.isDraw) {
            glFinish();
            glReadBuffer(GL_BACK_LEFT);
            glReadPixels(
                0,
                0,
                WND_WIDTH_FHD,
                WND_HEIGHT_FHD,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                videoBuffer.data()
            );
            for(auto line = 0; line != (WND_HEIGHT_FHD>>1); ++line) {
                std::swap_ranges(
                        videoBuffer.begin() + (4 * WND_WIDTH_FHD * line),
                        videoBuffer.begin() + (4 * WND_WIDTH_FHD * (line+1)),
                        videoBuffer.begin() + (4 * WND_WIDTH_FHD * (WND_HEIGHT_FHD-line-1)));
            }            
            publish_instance.capture_->Render(
                (char*)videoBuffer.data(),
                videoBuffer.size(),
                WND_WIDTH_FHD,
                WND_HEIGHT_FHD
            );
        }
#else
        // glFinish();
        // glReadBuffer(GL_BACK_LEFT);
        glReadPixels(
            0,
            0,
            WND_WIDTH_FHD,
            WND_HEIGHT_FHD,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            videoBuffer.data()
        );
        bres::bm_add_pipeline_frame(
            videoBuffer.data(),
            videoBuffer.size()
        );
#endif
        //
        if (((now - prev_usec) >= 1000000.0 /*  1000ms */)) {
            bres::sevent_t ev{bres::EVENT_TYPE_NONE, ""};
            {
                bres::LOCK lock(ipc_events.mtx_);
                if (!ipc_events.events_.empty()) {
                    ev = ipc_events.events_.front();
                    ipc_events.events_.pop();
                }
            }
            if (ev.type == bres::EVENT_TYPE_SUBSCRIBE_ADD_CHANNEL) {
                bres::subscribeSideAddChannel(pooled_instances, ev.event, 0);
            } else if (ev.type == bres::EVENT_TYPE_SUBSCRIBE_DELETE_CHANNEL) {
                bres::subscribeSideDeleteChannel(pooled_instances, ev.event, 0);
            } else if (ev.type == bres::EVENT_TYPE_SUBSCRIBE_PROPERTY_CHANNEL) {
                bres::subscribeSidePropertyChannel(pooled_instances, ev.event, 0);
            } else if (ev.type == bres::EVENT_TYPE_RESOURCE_ADD_STATIC_IMAGE) {
                bres::resourceAddStaticImage(pooled_instances, ev.event, 0);
            } else if (ev.type == bres::EVENT_TYPE_RESOURCE_DELETE_STATIC_IMAGE) {
                bres::resourceDeleteStaticImage(pooled_instances, ev.event, 0);
            }
            prev_usec = now;
        }
    }
    LOG("<<< main loop.");
    bres::publishSideDeleteChannel(publish_instance, "", 0);
    // cleanup
    glfwTerminate();
    //
    for(auto& inst : pooled_instances) {
        inst.quit = true;
    }
    bres::quit = true;
    for(auto &th : threads) {
        th.join();
    }
    //
    for(auto& inst : pooled_instances) {
        if (inst.instance_enum != bres::INSTANCE_MIN) {
            bres::websockets_quit(&inst, inst.context);
            bres::webrtc_quit(&inst);
            bres::gl_quit(&inst);    
        }
    }
    closelog();
    //
    return(0);
}
