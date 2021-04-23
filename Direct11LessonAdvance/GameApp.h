#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Geometry.h"
#include "LightHelper.h"
#include "Camera.h"
#include "GameObject.h"
#include "ConstantStruct.h"


class GameApp : public D3DApp
{
public:
	typedef VertexPosNormalTex VertexType ;
	typedef DWORD IndexType;

	enum class CameraMode { FirstPerson, ThirdPerson, Free };

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitGraphic();
	bool InitScene();
	bool InitCamera();
	bool InitLight();
	bool InitMaterial();

	std::shared_ptr<GameObject> BuildGameObject(
		std::shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData,
		std::wstring vShaderPath, std::wstring vShaderFunc,
		std::wstring pShaderPath, std::wstring pShaderFunc, std::wstring TexturePath);

private:
	// helper func for BuildGameObject
	std::wstring GetShaderCSOPath(std::wstring vShaderPath);

	void AppBuildVertexBuffer(D3D11_SUBRESOURCE_DATA& InitData, ComPtr<ID3D11Buffer> pVertexBuffer); // 为了兼容以前的版本所以保留的方法
	void AppBuildIndexBuffer(D3D11_SUBRESOURCE_DATA& InitData, ComPtr<ID3D11Buffer> pIndexBuffer);

	void AppBuildVertexBuffer(std::shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData, ComPtr<ID3D11Buffer>& pVertexBuffer);
	void AppBuildIndexBuffer(std::shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData, ComPtr<ID3D11Buffer>& pIndexBuffer);
	void AppBuildSamplerState(ComPtr<ID3D11SamplerState>& pSamplerState);

	bool InitSceneConstantBuffer();
	void AppCreateConstantBuffer();
	void AppSetVSConstantBuffer();
	void AppSetPSConstantBuffer();

	void AppBuildLightConstant();
	void AppBuildPSMultLightConstant();
	void AppBuildMaterialConstant();
	void AppBuildCameraConstant();
	void AppBuildPSTextureResource();



private:
	std::vector<std::shared_ptr<Geometry::MeshData<VertexType, IndexType>>> m_pMeshDataList;
	std::vector<std::shared_ptr<GameObject>> m_GameObjectList;
	std::shared_ptr<Graphic> m_pGraphic;

private:

	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    
	ComPtr<IDWriteFont> m_pFont;								
	ComPtr<IDWriteTextFormat> m_pTextFormat;					

	ComPtr<ID3D11InputLayout> m_pVertexLayout2D;				
	ComPtr<ID3D11InputLayout> m_pVertexLayout3D;				
	ComPtr<ID3D11Buffer> m_pConstantBuffers[4];				    
	//0 CBDrawing 物体变换等常量 1 CBFrame 观察矩阵  2 CBOnResize 窗口大小 3 CBRarely 灯光材质
					

	ComPtr<ID3D11VertexShader> m_pVertexShader3D;				
	ComPtr<ID3D11PixelShader> m_pPixelShader3D;				   
	ComPtr<ID3D11VertexShader> m_pVertexShader2D;				
	ComPtr<ID3D11PixelShader> m_pPixelShader2D;				   

	CBChangesEveryFrame m_CBFrame;							    // 该缓冲区存放仅在每一帧进行更新的变量
	CBChangesOnResize m_CBOnResize;							    // 该缓冲区存放仅在窗口大小变化时更新的变量
	CBChangesRarely m_CBRarely;								    // 该缓冲区存放不会再进行修改的变量

	ComPtr<ID3D11SamplerState> m_pSamplerState;				   

	std::shared_ptr<Camera> m_pCamera;
	CameraMode m_CameraMode;									
};
#endif