#include "GameObject.h"
using namespace std;
using namespace DirectX;
GameObject::GameObject(shared_ptr<Graphic> pGraphic)
{
	m_pGraphic = pGraphic;
}

GameObject::GameObject(const GameObject& object)
{
	m_pGraphic = object.m_pGraphic;

	m_Transform = object.m_Transform;

	m_pVertexBuffer = object.m_pVertexBuffer;
	m_pVertexLayout = object.m_pVertexLayout;
	m_pIndexBuffer = object.m_pIndexBuffer;
	m_pConstantBuffer = object.m_pConstantBuffer;
	m_pVertexShader = object.m_pVertexShader;
	m_pPixelShader = object.m_pPixelShader;

	m_pSamplerState = object.m_pSamplerState;
	m_pTexture = object.m_pTexture;

	VertexShaderFilePath = object.VertexShaderFilePath;
	VertexShaderFuncName = object.VertexShaderFuncName;
	PixelShaderFilePath = object.PixelShaderFilePath;
	PixelShaderFuncName = object.PixelShaderFuncName;

	m_pObjectMeshData = object.m_pObjectMeshData;

	m_VertexStride = object.m_VertexStride;
	m_IndexCount = object.m_IndexCount;
}



void GameObject::InitGameObjecct()
{
}

void GameObject::SetRawMesh(shared_ptr<Geometry::MeshData<VertexType, IndexType>> pObjectMeshData)
{
	m_pObjectMeshData = pObjectMeshData;
	m_VertexStride = sizeof(VertexType);
	m_IndexCount = (UINT)(pObjectMeshData->indexVec.size());
}

void GameObject::SetTextureName(wstring TexturePath)
{
}

void GameObject::SetShaderName(wstring vShaderPath, wstring vShaderFunc, wstring pShaderPath, wstring pShaderFunc)
{
}

void GameObject::SetVertexBuffer(ComPtr<ID3D11Buffer> pVertexBuffer)
{
	m_pVertexBuffer = pVertexBuffer;
}

void GameObject::SetLayout(ComPtr<ID3D11InputLayout> pVertexLayout)
{
	m_pVertexLayout = pVertexLayout;
}

void GameObject::SetIndexBuffer(ComPtr<ID3D11Buffer> pIndexBuffer)
{
	m_pIndexBuffer = pIndexBuffer;
}

void GameObject::SetVertexShader(ComPtr<ID3D11VertexShader> pVertexShader)
{
	m_pVertexShader = pVertexShader;
}

void GameObject::SetPixelShader(ComPtr<ID3D11PixelShader> pPixelShader)
{
	m_pPixelShader = pPixelShader;
}

void GameObject::SetTextureResourceView(ComPtr<ID3D11ShaderResourceView> pTexture)
{
	m_pTexture = pTexture;
}

void GameObject::SetSamplerState(ComPtr<ID3D11SamplerState> pSamplerState)
{
	m_pSamplerState = pSamplerState;
}

void GameObject::SetConstantBuffer(ComPtr<ID3D11Buffer> pConstantBuffer)
{
	m_pConstantBuffer = pConstantBuffer;
}

Transform& GameObject::GetTransform()
{
	// TODO: 在此处插入 return 语句
	return m_Transform;
}

const Transform& GameObject::GetTransform() const
{
	// TODO: 在此处插入 return 语句
	return m_Transform;
}

void GameObject::Draw()
{
	UINT strides = m_VertexStride;
	UINT offsets = 0;
	m_pGraphic->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pGraphic->GetContext()->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
	m_pGraphic->GetContext()->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_pGraphic->GetContext()->IASetInputLayout(m_pVertexLayout.Get());
	m_pGraphic->GetContext()->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pGraphic->GetContext()->PSSetShader(m_pPixelShader.Get(), nullptr, 0);


	ComPtr<ID3D11Buffer> cBuffer = nullptr;
	m_pGraphic->GetContext()->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	CBChangesEveryDrawing cbDrawing;
	// 内部进行转置
	XMMATRIX W = m_Transform.GetLocalToWorldMatrixXM();
	cbDrawing.world = XMMatrixTranspose(W);
	cbDrawing.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

	// 更新常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pGraphic->GetContext()->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryDrawing), &cbDrawing, sizeof(CBChangesEveryDrawing));
	m_pGraphic->GetContext()->Unmap(cBuffer.Get(), 0);

	// 设置纹理
	m_pGraphic->GetContext()->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
	m_pGraphic->GetContext()->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
	// 可以开始绘制
	m_pGraphic->GetContext()->DrawIndexed(m_IndexCount, 0, 0);
}
