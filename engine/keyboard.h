
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "input.h"

struct KeyControl;// : public InputControl;

struct Keyboard : public InputDevice {
    Keyboard();
    ~Keyboard();

    // no-op.
    virtual void Update() {}
    virtual void Unpress();

    virtual InputControl* GetControl(const std::string& name);

    // Actual processing goes on here.
    void KeyDown(uint keyCode);
    void KeyUp(uint keyCode);

    bool WasKeyPressed() const;
    char GetKey();
    void ClearKeyQueue();

private:
    std::queue<char> _keyQueue;

    typedef std::map<std::string, uint> KeySymMap;
    typedef std::map<uint, KeyControl*> KeyMap;

    KeySymMap _nameToKeySym;
    KeyMap _keys;
};

struct KeyControl : public InputControl {
    KeyControl();

    void KeyDown();
    void KeyUp();

protected:
    virtual float GetPosition();

private:
    bool _pressed;
};

#endif
