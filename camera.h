#ifndef CAMERA_H
#define CAMERA_H

// 通过矩阵进行变换的必备头包
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 定义一个摄像机的移动枚举 便于让摄像机移动从输入抽离
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// 默认相机参数
const float YAW = -90.0f; // 相机初始状态下看向-Z轴
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f; // 稀释DPI的硬件灵敏度
const float ZOOM = 45.0f; // 基础视窗大小

// 抽象出来的相机类 提供一个Update不断更新处理 鼠标键盘输入 提供最新的LookAt矩阵 让外部实现观察空间的转换
class Camera {
public:
	// 相机属性
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 WorldUp; // 世界坐标系的上部
	glm::vec3 Up; // 相机的Y轴归一化向量
	glm::vec3 Right; // 相机的右轴

	// 视角属性
	float Yaw;
	float Pitch;
	float Zoom;

	// 参数配置
	float MovementSpeed;
	float MouseSensitivity;

	// 构造函数 传递vec3坐标版本
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) :
		Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY)
	{
		Position = position;
		Yaw = yaw;
		Pitch = pitch;
		WorldUp = up;
		Zoom = ZOOM;
		updateCameraVectors();
	}

	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw = YAW, float pitch = PITCH) :
		Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY) 
	{
		Position = glm::vec3(posX, posY, posZ);
		Yaw = yaw;
		Pitch = pitch;
		WorldUp = glm::vec3(upX, upY, upZ);
		updateCameraVectors();
	}
	
	// 核心API 获取视角变换矩阵
	glm::mat4 GetViewMatrix() 
	{
		return glm::lookAt(Position, Position + Front, Up); // 通过Position + Front 得到相机的看向目标
	}

	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		switch (direction) {
		case FORWARD:
			Position += Front * velocity;
			break;
		case BACKWARD:
			Position -= Front * velocity;
			break;
		case RIGHT:
			Position += Right * velocity;
			break;
		case LEFT:
			Position -= Right * velocity;
			break;
		}
	}

	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) 
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		if (Pitch > 89.0f) {
			Pitch = 89.0f;
		}
		else if (Pitch < -89.0f) 
		{
			Pitch = -89.0f;
		}
		
		updateCameraVectors();
	}

	void ProcessMouseScroll(float yoffset) 
	{
		Zoom -= yoffset;

		if (Zoom < 1.0f) 
		{
			Zoom = 1.0f;
		}
		else if (Zoom > 45.0f) 
		{
			Zoom = 45.0f;
		}
	}

private:
	// Update函数 持续更新相机的前向坐标
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// 更新上轴与左轴
		Right = glm::normalize(glm::cross(Front, WorldUp)); // 因为我们限制了Roll 所以WorldUp和Up都天然的与上轴和左轴垂直
		Up = glm::normalize(glm::cross(Right, Front));
	}
};

#endif
