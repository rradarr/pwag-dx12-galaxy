#include "stdafx.h"
#include "EngineObject.h"


EngineObject::EngineObject(int index, Mesh mesh) {
	this->idx = index;
	this->mesh = mesh;
	this->delta_rotXMat = DirectX::XMMatrixRotationX(0.f);
	this->delta_rotYMat = DirectX::XMMatrixRotationX(0.f);
	this->delta_rotZMat = DirectX::XMMatrixRotationX(0.f);
}