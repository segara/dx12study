#pragma once

#include "../Engine/Resource/Mesh.h"
#include "../Engine/Resource/Shader.h" // Shader 헤더 추가

class Game
{
public:
    void Init(const WindowInfo& info);
    void Update();

private:
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(); // shader 멤버 추가
};

