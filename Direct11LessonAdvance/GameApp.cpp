#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::FirstPerson),
	m_CBFrame(),
	m_CBOnResize(),
	m_CBRarely()
{

}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;
	if (!InitGraphic())
		return false;
	if (!InitScene())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			L"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. 使用别的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK)
	{
		// 创建固定颜色刷和文本格式
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(m_pd2dRenderTarget);
	}

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);
	}
}

void GameApp::UpdateScene(float dt)
{
	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	// 获取子类
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

	Transform& woodCrateTransform = m_GameObjectList[0]->GetTransform();

	if (m_CameraMode == CameraMode::FirstPerson || m_CameraMode == CameraMode::Free)
	{
		// 第一人称/自由摄像机的操作

		// 方向移动
		if (keyState.IsKeyDown(Keyboard::W))
		{
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * 6.0f);
			else
				cam1st->MoveForward(dt * 6.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S))
		{
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * -6.0f);
			else
				cam1st->MoveForward(dt * -6.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -6.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 6.0f);

		// 将摄像机位置限制在[-8.9, 8.9]x[-8.9, 8.9]x[0.0, 8.9]的区域内
		// 不允许穿地
		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorSet(-8.9f, 0.0f, -8.9f, 0.0f), XMVectorReplicate(8.9f)));
		cam1st->SetPosition(adjustedPos);

		// 仅在第一人称模式移动摄像机的同时移动箱子
		if (m_CameraMode == CameraMode::FirstPerson)
			woodCrateTransform.SetPosition(adjustedPos);
		// 在鼠标没进入窗口前仍为ABSOLUTE模式
		if (mouseState.positionMode == Mouse::MODE_RELATIVE)
		{
			cam1st->Pitch(mouseState.y * dt * 2.5f);
			cam1st->RotateY(mouseState.x * dt * 2.5f);
		}

	}
	else if (m_CameraMode == CameraMode::ThirdPerson)
	{
		// 第三人称摄像机的操作

		cam3rd->SetTarget(woodCrateTransform.GetPosition());

		// 绕物体旋转
		cam3rd->RotateX(mouseState.y * dt * 2.5f);
		cam3rd->RotateY(mouseState.x * dt * 2.5f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	// 更新观察矩阵
	XMStoreFloat4(&m_CBFrame.eyePos, m_pCamera->GetPositionXM());
	m_CBFrame.view = XMMatrixTranspose(m_pCamera->GetViewXM());

	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();

	// 摄像机模式切换
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1) && m_CameraMode != CameraMode::FirstPerson)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam1st;
		}

		cam1st->LookTo(woodCrateTransform.GetPosition(),
			XMFLOAT3(0.0f, 0.0f, 1.0f),
			XMFLOAT3(0.0f, 1.0f, 0.0f));

		m_CameraMode = CameraMode::FirstPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_CameraMode != CameraMode::ThirdPerson)
	{
		if (!cam3rd)
		{
			cam3rd.reset(new ThirdPersonCamera);
			cam3rd->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam3rd;
		}
		XMFLOAT3 target = woodCrateTransform.GetPosition();
		cam3rd->SetTarget(target);
		cam3rd->SetDistance(8.0f);
		cam3rd->SetDistanceMinMax(3.0f, 20.0f);

		m_CameraMode = CameraMode::ThirdPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3) && m_CameraMode != CameraMode::Free)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam1st;
		}
		// 从箱子上方开始
		XMFLOAT3 pos = woodCrateTransform.GetPosition();
		XMFLOAT3 to = XMFLOAT3(0.0f, 0.0f, 1.0f);
		XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
		pos.y += 3;
		cam1st->LookTo(pos, to, up);

		m_CameraMode = CameraMode::Free;
	}
	// 退出程序，这里应向窗口发送销毁信息
	if (keyState.IsKeyDown(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryFrame), &m_CBFrame, sizeof(CBChangesEveryFrame));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//
	// 绘制几何模型
	//
	for (auto pObject : m_GameObjectList)
		pObject->Draw();

	

	//
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		wstring text = L"Switch Camera Mode: 1-First 2-Second 3-Free"; //L"切换摄像机模式: 1-第一人称 2-第三人称 3-自由视角\n";
			//L"W/S/A/D 前进/后退/左平移/右平移 (第三人称无效)  Esc退出\n"
			//L"鼠标移动控制视野 滚轮控制第三人称观察距离\n"
			//L"当前模式: ";
		//if (m_CameraMode == CameraMode::FirstPerson)
		//	text += L"第一人称(控制箱子移动)";
		//else if (m_CameraMode == CameraMode::ThirdPerson)
		//	text += L"第三人称";
		//else
		//	text += L"自由视角";
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitGraphic()
{
	m_pGraphic = shared_ptr<Graphic>(new Graphic(m_pd3dImmediateContext, m_pd3dDevice));
	return true;
}

bool GameApp::InitScene()
{
	InitSceneConstantBuffer();
	InitCamera();
	InitLight();
	InitMaterial();

	shared_ptr<Geometry::MeshData<VertexType, IndexType>> pBox 
		= make_shared<Geometry::MeshData<VertexType, IndexType>>(Geometry::CreateBox());
	m_pMeshDataList.push_back(pBox);
	shared_ptr<GameObject> pWood = BuildGameObject(pBox,
		wstring(L"HLSL\\Basic_VS_3D.hlsl"), wstring(L"VS_3D"),
		wstring(L"HLSL\\Basic_PS_3D.hlsl"), wstring(L"PS_3D"), wstring(L"..\\Texture\\WoodCrate.dds"));
	m_GameObjectList.push_back(pWood);

	shared_ptr<Geometry::MeshData<VertexType, IndexType>> pPlane
		= make_shared<Geometry::MeshData<VertexType, IndexType>>(Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	m_pMeshDataList.push_back(pPlane);
	shared_ptr<GameObject> pFloor = BuildGameObject(pPlane,
		wstring(L"HLSL\\Basic_VS_3D.hlsl"), wstring(L"VS_3D"),
		wstring(L"HLSL\\Basic_PS_3D.hlsl"), wstring(L"PS_3D"), wstring(L"..\\Texture\\floor.dds"));
	pFloor->GetTransform().SetPosition(0.0f, -1.0f, 0.0f);
	m_GameObjectList.push_back(pFloor);

	return true;
}

bool GameApp::InitCamera()
{
	m_CameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->LookAt(XMFLOAT3(), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	AppBuildCameraConstant();
	return true;
}

bool GameApp::InitLight()
{
	AppBuildLightConstant();
	return true;
}

bool GameApp::InitMaterial()
{
	AppBuildMaterialConstant();
	return true;
}

shared_ptr<GameObject> GameApp::BuildGameObject(shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData,
	wstring vShaderPath, wstring vShaderFunc, 
	wstring pShaderPath,  wstring pShaderFunc, wstring TexturePath)
{
	shared_ptr<GameObject> pNewObject = shared_ptr<GameObject>(new GameObject(m_pGraphic));

	ComPtr<ID3D11Buffer> pVertexBuffer;
	ComPtr<ID3D11Buffer> pIndexBuffer;
	ComPtr<ID3D11InputLayout> pVertexLayout;
	
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	ComPtr<ID3D11SamplerState> pSamplerState;
	ComPtr<ID3D11ShaderResourceView> pTexture;

	ComPtr<ID3DBlob> blob;

	AppBuildVertexBuffer(pMeshData, pVertexBuffer);
	AppBuildIndexBuffer(pMeshData, pIndexBuffer);

	
	HR(CreateShaderFromFile(GetShaderCSOPath(vShaderPath).c_str(), vShaderPath.c_str(), (LPCSTR)vShaderFunc.c_str(), "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pVertexShader.GetAddressOf())); // 先要创建着色器
	
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pVertexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(GetShaderCSOPath(pShaderPath).c_str(), pShaderPath.c_str(), (LPCSTR)pShaderFunc.c_str(), "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pPixelShader.GetAddressOf()));
	
	AppBuildSamplerState(pSamplerState);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), TexturePath.c_str(), nullptr, pTexture.GetAddressOf()));


	pNewObject->SetRawMesh(pMeshData);
	pNewObject->SetVertexBuffer(pVertexBuffer);
	pNewObject->SetIndexBuffer(pIndexBuffer);
	pNewObject->SetLayout(pVertexLayout);
	pNewObject->SetVertexShader(pVertexShader);
	pNewObject->SetPixelShader(pPixelShader);
	pNewObject->SetSamplerState(pSamplerState);
	pNewObject->SetTextureResourceView(pTexture);

	return pNewObject;
}


wstring GameApp::GetShaderCSOPath(wstring vShaderPath)
{
	int pSuffix = vShaderPath.find_last_of(L'.');
	wstring cso_path = vShaderPath.substr(0, pSuffix).append(L".cso");
	return cso_path;
}

void GameApp::AppBuildVertexBuffer(D3D11_SUBRESOURCE_DATA& InitData, ComPtr<ID3D11Buffer> m_pVertexBuffer)
{
	// ******************
	// 设置立方体顶点
	//    5________ 6
	//    /|      /|
	//   /_|_____/ |
	//  1|4|_ _ 2|_|7
	//   | /     | /
	//   |/______|/
	//  0       3
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) }
	};

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(vertices);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));
}

void GameApp::AppBuildIndexBuffer(D3D11_SUBRESOURCE_DATA& InitData, ComPtr<ID3D11Buffer> m_pIndexBuffer)
{
	// 索引数组
	DWORD indices[] = {
		// 正面
		0, 1, 2,
		2, 3, 0,
		// 左面
		4, 5, 1,
		1, 0, 4,
		// 顶面
		1, 5, 6,
		6, 2, 1,
		// 背面
		7, 6, 5,
		5, 4, 7,
		// 右面
		3, 2, 6,
		6, 7, 3,
		// 底面
		4, 0, 3,
		3, 7, 4
	};
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(indices);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	InitData.pSysMem = indices;
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
}

void GameApp::AppBuildVertexBuffer(shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData, ComPtr<ID3D11Buffer>& m_pVertexBuffer)
{
	m_pVertexBuffer.Reset();
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)pMeshData->vertexVec.size() * sizeof(VertexType);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pMeshData->vertexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));
}

void GameApp::AppBuildIndexBuffer(shared_ptr<Geometry::MeshData<VertexType, IndexType>> pMeshData, ComPtr<ID3D11Buffer>& m_pIndexBuffer)
{
	m_pIndexBuffer.Reset();
	int IndexCount = (UINT)pMeshData->indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = IndexCount * sizeof(DWORD);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pMeshData->indexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
}

void GameApp::AppBuildSamplerState(ComPtr<ID3D11SamplerState>& pSamplerState)
{
	// 初始化采样器状态
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(m_pd3dDevice->CreateSamplerState(&sampDesc, pSamplerState.GetAddressOf()));
}

bool GameApp::InitSceneConstantBuffer()
{
	AppCreateConstantBuffer();
	AppSetVSConstantBuffer();
	AppSetPSConstantBuffer();
	return true;
}

void GameApp::AppCreateConstantBuffer()
{
	// 设置常量缓冲区描述
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 新建用于VS和PS的常量缓冲区
	cbd.ByteWidth = sizeof(CBChangesEveryDrawing);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesEveryFrame);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesOnResize);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[2].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesRarely);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[3].GetAddressOf()));
}

void GameApp::AppSetVSConstantBuffer()
{
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(2, 1, m_pConstantBuffers[2].GetAddressOf());
}

void GameApp::AppSetPSConstantBuffer()
{
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(3, 1, m_pConstantBuffers[3].GetAddressOf());
}

void GameApp::AppBuildLightConstant()
{
	// 初始化不会变化的值
	// 环境光
	m_CBRarely.dirLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_CBRarely.dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// 灯光
	m_CBRarely.pointLight[0].position = XMFLOAT3(0.0f, 10.0f, 0.0f);
	m_CBRarely.pointLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.pointLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_CBRarely.pointLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.pointLight[0].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_CBRarely.pointLight[0].range = 25.0f;
	m_CBRarely.numDirLight = 1;
	m_CBRarely.numPointLight = 1;
	m_CBRarely.numSpotLight = 0;

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesRarely), &m_CBRarely, sizeof(CBChangesRarely));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);
}

void GameApp::AppBuildPSMultLightConstant()
{
}

void GameApp::AppBuildMaterialConstant()
{
	// 初始化材质
	m_CBRarely.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.material.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	m_CBRarely.material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 50.0f);

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesRarely), &m_CBRarely, sizeof(CBChangesRarely));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);
}

void GameApp::AppBuildCameraConstant()
{
	m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);
}

void GameApp::AppBuildPSTextureResource()
{
}