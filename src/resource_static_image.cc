#include "bres+client_ext.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//
void
bres::resourceAddStaticImage(
    std::vector<bres::instance_t>& pooled_instances,
    std::string ev,
    unsigned padding
)
{
    unsigned instance_idx = bres::INSTANCE_MIN;
    int w,h,f;
    //
    for(auto& inst : pooled_instances) {
        if (inst.instance_enum == bres::INSTANCE_MIN) {
            break;
        }
        instance_idx ++;
    }
    auto json = nlohmann::json::parse(ev);
    if (json.find("path") == json.end() ||
        json.find("pos_x") == json.end() ||
        json.find("pos_y") == json.end() ||
        instance_idx >= bres::INSTANCE_MAX) {
        LOG("invalid args.");
        return;
    }
    auto path = json["path"].get<std::string>();
    auto pos_x = json["pos_x"].get<std::string>();
    auto pos_y = json["pos_y"].get<std::string>();
    //
    auto image = stbi_load(path.c_str(), &w, &h, &f, 0);
    if (!image) {
        LOG("not found image(%s).", path.c_str());
        return;
    }
    //
    auto& inst = pooled_instances.at(instance_idx);
    inst.instance_enum = bres::INSTANCE_LOCAL_00;
    inst.quit = false;
    inst.isDraw = 1;
    inst.posX = std::stof(pos_x);
    inst.posY = std::stof(pos_y);
    inst.width = w;
    inst.height = h;
    inst.init_image = image;
    inst.image_path = path;
    inst.scale = 1.f;
    inst.order_ = bres::INSTANCE_ORDER_MIN;
    inst.lastTextureBuffer_ = std::vector<char>(inst.width * inst.height * 4);
    memcpy(inst.lastTextureBuffer_.data(), image, inst.lastTextureBuffer_.size());
    //
    bres::gl_generate_texture(&inst);
    if (image) {
        stbi_image_free(image);
    }
}
//
void
bres::resourceDeleteStaticImage(
    std::vector<bres::instance_t>& pooled_instances,
    std::string ev,
    unsigned instance_idx
)
{
    LOG("resourceDeleteStaticImage(%s)", ev.c_str());
    auto json = nlohmann::json::parse(ev);
    if (json.find("path") == json.end()) {
        LOG("invalid args.");
        return;
    }
    auto path = json["path"].get<std::string>();

    for(auto& inst : pooled_instances) {
        if (!inst.image_path.compare(path)) {
            inst.quit = true;
            inst.isDraw = 0;
            inst.instance_enum = bres::INSTANCE_MIN;
            bres::gl_quit(&inst);
        }
    }
}
