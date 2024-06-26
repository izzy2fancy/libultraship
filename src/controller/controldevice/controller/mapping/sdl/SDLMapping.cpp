#include "SDLMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

#include <Utils/StringHelper.h>

namespace LUS {
SDLMapping::SDLMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex), mController(nullptr) {
}

SDLMapping::~SDLMapping() {
}

bool SDLMapping::OpenController() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        mController = nullptr;
        return false;
    }

    const auto newCont = SDL_GameControllerOpen(deviceIndexMapping->GetSDLDeviceIndex());

    // We failed to load the controller
    if (newCont == nullptr) {
        return false;
    }

    mController = newCont;
    return true;
}

bool SDLMapping::CloseController() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}

bool SDLMapping::ControllerLoaded() {
    SDL_GameControllerUpdate();

    // If the controller is disconnected, close it.
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        CloseController();
    }

    // Attempt to load the controller if it's not loaded
    if (mController == nullptr) {
        // If we failed to load the controller, don't process it.
        if (!OpenController()) {
            return false;
        }
    }

    return true;
}

SDL_GameControllerType SDLMapping::GetSDLControllerType() {
    if (!ControllerLoaded()) {
        return SDL_CONTROLLER_TYPE_UNKNOWN;
    }

    return SDL_GameControllerGetType(mController);
}

bool SDLMapping::UsesPlaystationLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 || type == SDL_CONTROLLER_TYPE_PS5;
}

bool SDLMapping::UsesSwitchLayout() {
#ifdef __SWITCH__
    return true;
#else
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO || type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
#endif
}

bool SDLMapping::UsesXboxLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_XBOX360 || type == SDL_CONTROLLER_TYPE_XBOXONE;
}

int32_t SDLMapping::GetSDLDeviceIndex() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        return -1;
    }

    return deviceIndexMapping->GetSDLDeviceIndex();
}

std::string SDLMapping::GetSDLControllerName() {
    return LUS::Context::GetInstance()
        ->GetControlDeck()
        ->GetDeviceIndexMappingManager()
        ->GetSDLControllerNameFromLUSDeviceIndex(mLUSDeviceIndex);
}

std::string SDLMapping::GetSDLDeviceName() {
    return ControllerLoaded()
               ? StringHelper::Sprintf("%s (SDL %d)", GetSDLControllerName().c_str(), GetSDLDeviceIndex())
               : StringHelper::Sprintf("%s (Disconnected)", GetSDLControllerName().c_str());
}

int32_t SDLMapping::GetJoystickInstanceId() {
    if (mController == nullptr) {
        return -1;
    }

    return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(mController));
}

int32_t SDLMapping::GetCurrentSDLDeviceIndex() {
    if (mController == nullptr) {
        return -1;
    }

    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (SDL_JoystickInstanceID(joystick) == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(mController))) {
            SDL_JoystickClose(joystick);
            return i;
        }
        SDL_JoystickClose(joystick);
    }

    // didn't find one
    return -1;
}
} // namespace LUS
