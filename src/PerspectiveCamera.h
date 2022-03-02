#include <cmath>

class PerspectiveCamera {
public:
	PerspectiveCamera(float fov, float aspect, float near, float far) {
		m_fov = fov;
		m_zoom = 1;
		m_near = near;
		m_far = far;
		m_focus = 10;
		m_aspect = aspect;
		m_fileGauge = 35;
		m_filmOffset = 0;
	}

	void updateProjectionMatrix() {
		float near = m_near;
		float far = m_far;
		float top = near * tan(0.5f * m_fov * 3.14159265f / 180.0f) / m_zoom;
		float height = 2.0f * top;
		float width = m_aspect * height;
		float left = -0.5f * width;
		float bottom = top - height;
		float right = left + width;

		float x = 2 * near / (right - left);
		float y = 2 * near / (top - bottom);
		float a = (right + left) / (right - left);
		float b = (top + bottom) / (top - bottom);
		float c = -(far + near) / (far - near);
		float d = -2 * far * near / (far - near);
		projection[0] = x;
		projection[4] = 0;
		projection[8] = a; 
		projection[12] = 0;
		projection[1] = 0;
		projection[5] = y;
		projection[9] = b;
		projection[13] = 0;
		projection[2] = 0;
		projection[6] = 0;
		projection[10] = c;
		projection[14] = d;
		projection[3] = 0;
		projection[7] = 0;
		projection[11] = -1;
		projection[15] = 0;

		// inv
		float n11 = projection[0];
		float n21 = projection[1];
		float n31 = projection[2];
		float n41 = projection[3];
		float n12 = projection[4];
		float n22 = projection[5];
		float n32 = projection[6];
		float n42 = projection[7];
		float n13 = projection[8];
		float n23 = projection[9];
		float n33 = projection[10];
		float n43 = projection[11];
		float n14 = projection[12];
		float n24 = projection[13];
		float n34 = projection[14];
		float n44 = projection[15];
		float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
		float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
		float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
		float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;
		float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
		if (det == 0) {
			std::cout << "Wrong in Inv Mat4!\n";
		}
		float detInv = 1 / det;
		projectionInv[0] = t11 * detInv;
		projectionInv[1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
		projectionInv[2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
		projectionInv[3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;
		projectionInv[4] = t12 * detInv;
		projectionInv[5] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
		projectionInv[6] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
		projectionInv[7] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;
		projectionInv[8] = t13 * detInv;
		projectionInv[9] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
		projectionInv[10] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
		projectionInv[11] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;
		projectionInv[12] = t14 * detInv;
		projectionInv[13] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
		projectionInv[14] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
		projectionInv[15] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;
	}

public:
	float m_fov;
	float m_zoom;
	float m_near;
	float m_far;
	float m_focus;
	float m_aspect;
	float m_fileGauge;
	float m_filmOffset;
	float projection[16] = {};
	float projectionInv[16] = {};
};