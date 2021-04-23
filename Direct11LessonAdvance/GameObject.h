#pragma once
#include <sstream>
#include <xstring>
#include <vector>
#include "Transform.h"
#include "d3dApp.h"
#include "DXTrace.h"
#include "Geometry.h"
#include "d3dUtil.h"

#include "ConstantStruct.h"

class GameObject
{
public:
	GameObject(std::shared_ptr<Graphic> pGraphic);
	GameObject(const GameObject& object);
	void InitGameObjecct();

	void SetRawMesh(std::shared_ptr<Geometry::MeshData<VertexType, IndexType>> pObjectMeshData);
	void SetTextureName(std::wstring TexturePath);
	void SetShaderName(std::wstring vShaderPath, std::wstring vShaderFunc, std::wstring pShaderPath, std::wstring pShaderFunc);
	
	void SetVertexBuffer(ComPtr<ID3D11Buffer> pVertexBuffer);
	void SetLayout(ComPtr<ID3D11InputLayout> pVertexLayout);
	void SetIndexBuffer(ComPtr<ID3D11Buffer> pIndexBuffer);
	void SetVertexShader(ComPtr<ID3D11VertexShader> pVertexShader);
	void SetPixelShader(ComPtr<ID3D11PixelShader> pPixelShader);
	void SetTextureResourceView(ComPtr<ID3D11ShaderResourceView> pTexture);
	void SetSamplerState(ComPtr<ID3D11SamplerState> pSamplerState);

	void SetConstantBuffer(ComPtr<ID3D11Buffer> pConstantBuffer); // 现在还不需要，直接用GameApp的constantbuffer


	Transform& GetTransform();
	const Transform& GetTransform() const;

	void Draw();

private:
	std::shared_ptr<Graphic> m_pGraphic;
	
	Transform m_Transform;
	
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11InputLayout> m_pVertexLayout;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;
	ComPtr<ID3D11Buffer> m_pConstantBuffer;
	ComPtr<ID3D11VertexShader> m_pVertexShader;
	ComPtr<ID3D11PixelShader> m_pPixelShader;

	ComPtr<ID3D11SamplerState> m_pSamplerState;
	ComPtr<ID3D11ShaderResourceView> m_pTexture;		

	std::wstring VertexShaderFilePath;
	std::wstring VertexShaderFuncName;
	std::wstring PixelShaderFilePath;
	std::wstring PixelShaderFuncName;

	std::shared_ptr<Geometry::MeshData<VertexPosNormalTex, DWORD>> m_pObjectMeshData;

	UINT m_VertexStride;								
	UINT m_IndexCount;
	
	
};

