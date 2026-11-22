#include "ViewpointClass.h"

ViewPointClass::ViewPointClass()
{
}


ViewPointClass::ViewPointClass(const ViewPointClass& other)
{
}


ViewPointClass::~ViewPointClass()
{
}


void ViewPointClass::SetPosition(float x, float y, float z)
{
    m_position = XMFLOAT3(x, y, z);
}


void ViewPointClass::SetLookAt(float x, float y, float z)
{
    m_lookAt = XMFLOAT3(x, y, z);
}


void ViewPointClass::SetProjectionParameters(float fieldOfView, float aspectRatio, float nearPlane, float farPlane)
{
    m_fieldOfView = fieldOfView;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}


void ViewPointClass::GenerateViewMatrix()
{
    XMFLOAT3 up;
    XMVECTOR upVector, positionVector, lookAtVector;


    // Setup the vector that points upwards.
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;

    // Load XMFLOAT3 variables into a XMVECTOR structures.
    upVector = XMLoadFloat3(&up);
    positionVector = XMLoadFloat3(&m_position);
    lookAtVector = XMLoadFloat3(&m_lookAt);

    // Create the view matrix from the three vectors.
    m_viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

}


void ViewPointClass::GenerateProjectionMatrix()
{
    // Create the projection matrix for the view point.
    m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
}


void ViewPointClass::GetViewMatrix(XMMATRIX& viewMatrix)
{
    viewMatrix = m_viewMatrix;
}


void ViewPointClass::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
    projectionMatrix = m_projectionMatrix;
}
