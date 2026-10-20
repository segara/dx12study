#include "pch.h"
#include "Game.h"
#include "Engine/Engine.h"

void Game::Init(const WindowInfo& info)
{
	GEngine->Init(info);
	vector<Vertex> vec(3);
	vec[0].pos = Vec3(0.f, 0.5f, 0.5f);
	vec[0].color = Vec4(1.f,0.f, 0.f, 1.f);
	vec[1].pos = Vec3(0.5f, -0.5f, 0.5f);
	vec[1].color = Vec4(0.f,1.f, 0.f, 1.f);
	vec[2].pos = Vec3(-0.5f, -0.5f, 0.5f);
	vec[2].color = Vec4(0.f,0.f, 1.f, 1.f);
	mesh->Init(vec);
	shader->Init(L"../Resources/Shader/default.hlsli");
	GEngine->GetCommandQueue()->WaitSync();	
}

void Game::Update()
{
	GEngine->RenderBegin();

	shader->Update();	 //매프레임 pipeline state를 셋팅

	{
		Transform transform;
		transform.offset = Vec4(0.5f, 0.f, 0.f, 0.f);
		mesh->SetTransform(transform);
		mesh->Render();
	}

	{
		Transform transform;
		transform.offset = Vec4(0.f, 0.5f, 0.f, 0.f);
		mesh->SetTransform(transform);
		mesh->Render();
	}

	GEngine->RenderEnd();
}
