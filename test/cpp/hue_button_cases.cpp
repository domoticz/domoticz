int main() {
    Sensors sensors{{{"owner", false, 1, "1"}, {"owner", false, 2, "2"}}};
    CPhilipsHue hue{&sensors};
    Json::Value event;
    event["owner"]["rid"] = "owner";
    event["button"]["button_report"]["event"] = "short_release";
    hue.HandleSSEButton(event);
    assert(hue.updates == 0);
    event["metadata"]["control_id"] = 0;
    hue.HandleSSEButton(event);
    assert(hue.updates == 0);
    event["metadata"]["control_id"] = 2;
    hue.HandleSSEButton(event);
    assert(hue.updates == 1 && hue.level == 20);
    sensors.buttons.pop_back();
    event.removeMember("metadata");
    hue.HandleSSEButton(event);
    assert(hue.updates == 2 && hue.level == 10);
    sensors.buttons.push_back({"other-owner", false, 2, "2"});
    hue.HandleSSEButton(event);
    assert(hue.updates == 3 && hue.level == 10);
}
