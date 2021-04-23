#pragma once
#include "d3dApp.h"
#include "LightHelper.h"
struct CBChangesEveryDrawing
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
};

struct CBChangesEveryFrame
{
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT4 eyePos;
};

struct CBChangesOnResize
{
	DirectX::XMMATRIX proj;
};

struct CBChangesRarely
{
	DirectionalLight dirLight[10];
	PointLight pointLight[10];
	SpotLight spotLight[10];
	Material material;
	int numDirLight;
	int numPointLight;
	int numSpotLight;
	float pad;		// �����֤16�ֽڶ���
};

typedef VertexPosNormalTex VertexType;
typedef DWORD IndexType;