#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			m_Origin{ _origin },
			m_FovAngle{ _fovAngle }
		{}

		float m_FovAngle{ 90.f };
		float m_Fov{ tanf((m_FovAngle * TO_RADIANS) / 2.f) };

		Vector3 m_Forward{ Vector3::UnitZ };
		Vector3 m_Up{ Vector3::UnitY };
		Vector3 m_Right{ Vector3::UnitX };
		Vector3 m_RightLocal{ Vector3::UnitX };
		Matrix m_ProjectionMatrix{};

		float m_TotalPitch{};
		float m_TotalYaw{};
		const float m_RotationSpeed{ 8.f };
		const float m_MovementSpeed{ 10.f };
		const float m_BoostSpeed{ 10.f };

		float m_AspectRatio{ 0.f };
		Matrix m_WorldViewProjectionMatrix{};
		Matrix m_InvViewMatrix{};
		Matrix m_OnbMatrix{};
		Matrix m_ViewMatrix{};
		Vector3 m_Origin{ 0,0,0 };

		const float nearZ{ 0.1f };
		const float farZ{ 100.f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f })
		{
			m_FovAngle = _fovAngle;
			m_Fov = tanf((m_FovAngle * TO_RADIANS) / 2.f);

			m_Origin = _origin;
		}

		Matrix GetViewMatrix() { return m_ViewMatrix; };
		Matrix GetProjectionMatrix() { return m_ProjectionMatrix; };

		void CalculateViewMatrix()
		{
			////ONB => invViewMatrix
			//Vector3 right{ Vector3::Cross(Vector3::UnitY, m_Forward).Normalized() };
			//Vector3 upVector{ Vector3::Cross(m_Forward, right).Normalized() };

			//Vector4 up4{ upVector, 0 };
			//Vector4 right4{ right, 0 };
			//Vector4 forward4{ m_Forward, 0 };;
			//Vector4 position{ m_Origin, 1 };

			////Inverse(ONB) => ViewMatrix
			//Matrix onb{ right4, up4, forward4, position };
			////onbMatrix = onb;
			//m_ViewMatrix = onb.Inverse();

			////ViewMatrix => Matrix::CreateLookAtLH(...)
			//m_ViewMatrix = Matrix::CreateLookAtLH(m_Origin, m_Forward, m_Up);
			//m_InvViewMatrix = m_ViewMatrix.Inverse();
			////DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh

			//ONB => invViewMatrix
			Vector3 right{ Vector3::Cross(Vector3::UnitY, m_Forward).Normalized() };
			Vector3 upVector{ Vector3::Cross(m_Forward, right).Normalized() };

			Vector4 up4{ upVector, 0 };
			Vector4 right4{ right, 0 };
			Vector4 forward4{ m_Forward, 0 };;
			Vector4 position{ m_Origin, 1 };

			//Inverse(ONB) => ViewMatrix
			Matrix onb{ right4, up4, forward4, position };
			//m_ViewMatrix = onb.Inverse();

			m_ViewMatrix = Matrix::CreateLookAtLH(m_Origin, m_Forward, m_Up);
			m_InvViewMatrix = m_ViewMatrix.Inverse();
			m_OnbMatrix = onb;
		}

		void CalculateProjectionMatrix()
		{
			m_ProjectionMatrix = Matrix::CreatePerspectiveFovLH(m_Fov, m_AspectRatio, nearZ, farZ);
		}

		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Camera Update Logic
			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			Vector3 forwardSpeed{ m_Forward * deltaTime * m_MovementSpeed };
			Vector3 sideSpeed{ m_Right * deltaTime * m_MovementSpeed };
			Vector3 upSpeed{ m_Up * deltaTime * m_MovementSpeed };
			if (pKeyboardState[SDL_SCANCODE_LSHIFT]) {
				forwardSpeed *= m_BoostSpeed;
				sideSpeed *= m_BoostSpeed;
				upSpeed *= m_BoostSpeed;
			}


			const float rotSpeed{ deltaTime * m_RotationSpeed };
			if (SDL_BUTTON(mouseState) == 8) { //rmb
				m_TotalPitch -= static_cast<float>(mouseY) * rotSpeed;
				m_TotalYaw += static_cast<float>(mouseX) * rotSpeed;
			}
			else if (SDL_BUTTON(mouseState) == 1) { //lmb
				m_Origin += static_cast<float>(mouseY) * forwardSpeed * 13;
				m_TotalYaw -= static_cast<float>(mouseX) * rotSpeed;
			}
			else if (SDL_BUTTON(mouseState) == 16) { //lmb + rmb
				m_Origin += static_cast<float>(mouseY) * upSpeed * m_BoostSpeed;
			}

			m_Origin += pKeyboardState[SDL_SCANCODE_UP] * forwardSpeed;
			m_Origin += pKeyboardState[SDL_SCANCODE_W] * forwardSpeed;
			m_Origin -= pKeyboardState[SDL_SCANCODE_S] * forwardSpeed;
			m_Origin -= pKeyboardState[SDL_SCANCODE_DOWN] * forwardSpeed;

			m_Origin += pKeyboardState[SDL_SCANCODE_SPACE] * upSpeed;
			m_Origin -= pKeyboardState[SDL_SCANCODE_LCTRL] * upSpeed;

			m_Origin += pKeyboardState[SDL_SCANCODE_D] * sideSpeed;
			m_Origin += pKeyboardState[SDL_SCANCODE_RIGHT] * sideSpeed;
			m_Origin -= pKeyboardState[SDL_SCANCODE_A] * sideSpeed;
			m_Origin -= pKeyboardState[SDL_SCANCODE_LEFT] * sideSpeed;
			


			Matrix finalRot{ Matrix::CreateRotation(m_TotalPitch, m_TotalYaw, 0) };
			m_Forward = finalRot.TransformVector(Vector3::UnitZ);
			m_Forward.Normalize();
			
			m_RightLocal = finalRot.TransformVector(Vector3::UnitX);
			m_RightLocal.Normalize();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes

		}
	};
}
