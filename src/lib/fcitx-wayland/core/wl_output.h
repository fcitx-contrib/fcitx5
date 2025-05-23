#ifndef WL_OUTPUT_H_
#define WL_OUTPUT_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlOutput final {
public:
    static constexpr const char *interface = "wl_output";
    static constexpr const wl_interface *const wlInterface =
        &wl_output_interface;
    static constexpr const uint32_t version = 4;
    using wlType = wl_output;
    operator wl_output *() { return data_.get(); }
    WlOutput(wlType *data);
    WlOutput(WlOutput &&other) noexcept = delete;
    WlOutput &operator=(WlOutput &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &geometry() { return geometrySignal_; }
    auto &mode() { return modeSignal_; }
    auto &done() { return doneSignal_; }
    auto &scale() { return scaleSignal_; }
    auto &name() { return nameSignal_; }
    auto &description() { return descriptionSignal_; }

private:
    static void destructor(wl_output *);
    static const struct wl_output_listener listener;
    fcitx::Signal<void(int32_t, int32_t, int32_t, int32_t, int32_t,
                       const char *, const char *, int32_t)>
        geometrySignal_;
    fcitx::Signal<void(uint32_t, int32_t, int32_t, int32_t)> modeSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(int32_t)> scaleSignal_;
    fcitx::Signal<void(const char *)> nameSignal_;
    fcitx::Signal<void(const char *)> descriptionSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_output, &destructor> data_;
};
static inline wl_output *rawPointer(WlOutput *p) {
    return p ? static_cast<wl_output *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_OUTPUT_H_
