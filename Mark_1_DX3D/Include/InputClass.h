#pragma once

class InputClass
{
public:
    InputClass();
    InputClass(const InputClass&);
    ~InputClass();
    
    void Initialize();
    void key_down(unsigned int);
    void key_up(unsigned int);
    
    bool is_key_down(unsigned int);
    
    private:
    
    bool m_keys[256];
};
