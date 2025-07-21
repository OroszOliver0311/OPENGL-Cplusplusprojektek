//=============================================================================================
// Zöld háromszög: A framework.h osztályait felhasználó megoldás
//=============================================================================================
#include "framework.h"
// csúcspont árnyaló
const char* vertSource = R"(
	#version 330				
    precision highp float;

	layout(location = 0) in vec2 cP;	// 0. bemeneti regiszter

	void main() {
		gl_Position = vec4(cP.x, cP.y, 0, 1); 	// bemenet már normalizált eszközkoordinátákban
	}
)";

// pixel árnyaló
const char* fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;			// konstans szín
	out vec4 fragmentColor;		// pixel szín

	void main() {
		fragmentColor = vec4(color, 1); // RGB -> RGBA
	}
)";

const int winWidth = 600, winHeight = 600;

class Object : public Geometry<vec3> {
public:
	Object() : Geometry<vec3>() {}
	//megváltoztatás
    void AddNew(const vec3& nobject) { vtx.push_back(nobject); }
	//GPU-ra szinkronizáció
	void SyncGPU() { if (this->Vtx().empty()) return;updateGPU();}

};
class PointCollection : public Object {
public:
	PointCollection() : Object() {}
	//Új Pont

	//Közelségi keresése 
	vec3 SelectPoint(const vec3& mouse) {
		for (const auto& point : vtx) {
			float dx = mouse.x - point.x;
			float dy = mouse.y - point.y;
			float dz = mouse.z - point.z;
			float d = sqrt(dx * dx + dy * dy + dz * dz);
			if (d <= 0.019f) return point;

		}
		return vec3(0.0f, 0.0f, 0.0f);
	}
	//Felrajzolás kapott színnel(10 vastagság, max intenzitású piros)
	void DrawPoints(GPUProgram* prog) { glPointSize(10.0f); Draw(prog, GL_POINTS, vec3(1.0f, 0.0f, 0.0f)); }
};



class Line {
	vec3 p1, p2, dir, normalp1, normalp2;
	float A, B, C;
public:
	Line(const vec3& np1, const vec3& np2) {
		normalp1 = np1;
		normalp2 = np2;
		A = np2.y - np1.y;
		B = np1.x - np2.x;
		C = np2.x * np1.y - np1.x * np2.y;
		dir = normalize(np2 - np1);

		vec3 corner1 = vec3(-1.0f, (A - C) / B, 1.0f);
		vec3 corner2 = vec3(1.0f, (-A - C) / B, 1.0f);
		vec3 corner3 = vec3((B - C) / A, -1.0f, 1.0f);
		vec3 corner4 = vec3((-B - C) / A, 1.0f, 1.0f);
		std::vector<vec3> corners;
		if (corner1.y >= 1.0f || corner1.y <= -1.0f) corners.push_back(corner1);
		if (corner2.y >= 1.0f || corner2.y <= -1.0f) corners.push_back(corner2);
		if (corner3.x >= 1.0f || corner3.x <= -1.0f) corners.push_back(corner3);
		if (corner4.x >= 1.0f || corner4.x <= -1.0f) corners.push_back(corner4);
		if (A == 0.0f) {
			corners.clear();
			corners.push_back(vec3(-1.0f, np1.y, 1.0f));
			corners.push_back(vec3(1.0f, np1.y, 1.0f));
		}
		if (B == 0) {
			corners.clear();
			corners.push_back(vec3(np1.x, -1.0f, 1.0f));
			corners.push_back(vec3(np1.x, 1.0f, 1.0f));
		}
		
		p1 = corners[0];
		p2 = corners[1];

		printf("Line added\n");
		printf("Implicit: %fx+%fy+%f= 0\n", A, B, C);
		printf("Parametric:P(t) = (%f, %f) + (%f, %f)t)\n", normalp1.x, normalp1.y, dir.x, dir.y);
	}
	vec3 CrossPoint(const Line other) {

		float x = (other.C * B - C * other.B) / (A * other.B - other.A * B);
		float y = (C * other.A - other.C * A) / (other.B * A - B * other.A);
		return vec3(x, y, 1.0f);

	}
	bool onLine(const vec3& mouse) {
		float d = abs(A * mouse.x + B * mouse.y + C) / sqrt(A * A + B * B);
		if (d < 0.01f) { return true; }
		return false;
	}
	void Translate(vec3 np) {
		vec3 offset = np - (normalp1 + normalp2) / 2.0f;
		normalp1 += offset;
		normalp2 += offset;
		p1 += offset;
		p2 += offset;
		A = normalp2.y - normalp1.y;
		B = normalp1.x - normalp2.x;
		C = normalp2.x * normalp1.y - normalp1.x * normalp2.y;
		dir = normalize(normalp2 - normalp1);
		printf("%fx+%fy+%fc\n", A, B, C);
	}
	vec3 getP1() { return p1; }
	vec3 getP2() { return p2; }
	vec3 getDir() { return dir; }
	vec3 getNormalP1() { return normalp1; }
	vec3 getNormalP2() { return normalp2; }
	float getA() { return A; }
	float getB() { return B; }
	float getC() { return C; }
};

class LineCollection : public Object {
	std::vector<Line> lines;
public:
	LineCollection() : Object() {}
	//Új egyenes
	void AddNew(Line nline) {
		lines.push_back(nline);
		vtx.push_back(nline.getP1()); vtx.push_back(nline.getP2());
	}
	//Közelségi keresése	
	Line* SelectLine(const vec3& mouse) {
		for (size_t i = 0; i < lines.size(); i++) {
			if (lines[i].onLine(mouse)) { return &lines[i]; }
		}
		return nullptr;

	}
	int hanyadik(Line nline) {
		for (size_t i = 0; i < lines.size(); i++) {
			if (lines[i].getNormalP1().x == nline.getNormalP1().x && lines[i].getNormalP1().y == nline.getNormalP1().y &&
				lines[i].getNormalP2().x == nline.getNormalP2().x && lines[i].getNormalP2().y == nline.getNormalP2().y &&
				lines[i].getDir().x == nline.getDir().x && lines[i].getDir().y == nline.getDir().y) return i;
		}
		return - 1;
	}
	//Rajz
	void DrawLines(GPUProgram* prog) { glLineWidth(3.0f); Draw(prog, GL_LINES, vec3(0.0f, 1.0f, 1.0f)); }


};
class GreenTriangleApp : public glApp {

	PointCollection* points;
	GPUProgram* gpuProgram;	   // csúcspont és pixel árnyalók
	LineCollection* lines;
	std::vector<vec3> pointsbuffer;
	std::vector<Line*> linesbuffer;
	Line* selectedLine;
	bool moving = false;
	int mode;
public:
	GreenTriangleApp() : glApp("Green triangle") {}
	// Inicializáció, 
	void onInitialization() {
		points = new PointCollection();
		lines = new LineCollection();
		gpuProgram = new GPUProgram(vertSource, fragSource);

	}

	// Ablak újrarajzolás
	void onDisplay() {
		glClearColor(0.4f, 0.4f, 0.4f, 0.0f);     // háttér szín
		glClear(GL_COLOR_BUFFER_BIT); // rasztertár törlés
		glViewport(0, 0, winWidth, winHeight);
		//triangle->Draw(gpuProgram, GL_TRIANGLES, vec3(0.0f, 1.0f, 0.0f));
		lines->DrawLines(gpuProgram);
		points->DrawPoints(gpuProgram);

	}
	void onKeyboard(int key) {
		if (key == 'p') {  mode = 'p'; printf("Point creator\n"); }
		if (key == 'l') { mode = 'l'; printf("Line creator\n"); }
		if (key == 'm') { mode = 'm'; printf("Move\n"); }
		if (key == 'i') { mode = 'i'; printf("Intersect\n"); }
	}
	void onMousePressed(MouseButton button, int pX, int pY) {

		

		if (button == MOUSE_LEFT) {
			vec3 mouse = vec3((2.0f * pX) / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight, 1.0f);

			if (mode == 'p') {
				printf("Point: %f, %f added\n", mouse.x, mouse.y);
				points->AddNew(mouse);
				points->SyncGPU();
				refreshScreen();
			}
			if (mode == 'l') {

					if (points->SelectPoint(mouse).z == 0 && points->SelectPoint(mouse).y == 0 && points->SelectPoint(mouse).x == 0) return;
					if (!pointsbuffer.empty() && pointsbuffer[0].x == points->SelectPoint(mouse).x && pointsbuffer[0].y == points->SelectPoint(mouse).y) return;
					pointsbuffer.push_back(points->SelectPoint(mouse));
					if (pointsbuffer.size() == 2) {
						lines->AddNew(Line(pointsbuffer[0], pointsbuffer[1]));
						pointsbuffer.clear();
						lines->SyncGPU();
						refreshScreen();
					}
				
				}

			if (mode == 'i') {
				if (lines->SelectLine(mouse)) {
					Line* selectedLine = lines->SelectLine(mouse);
					if (selectedLine->onLine(mouse)) linesbuffer.push_back(selectedLine);
					if (linesbuffer.size() == 2) {
						points->AddNew(linesbuffer[0]->CrossPoint(*linesbuffer[1]));
						linesbuffer.clear();
						points->SyncGPU();
						refreshScreen();
					}
				}

			}
			if (mode == 'm') {
				if (lines->SelectLine(mouse)) {
					moving = true;
					selectedLine = lines->SelectLine(mouse);
				}
			}
		}


	}
	void onMouseMotion(int pX, int pY) {
		if (moving) {
			vec3 mouse = vec3(2.0f * pX / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight, 1.0f);

			int pos = lines->hanyadik(*selectedLine);
			if (selectedLine) {
				selectedLine->Translate(mouse);
				lines->Vtx()[pos * 2] = selectedLine->getP1();
				lines->Vtx()[pos * 2 + 1] = selectedLine->getP2();
				lines->SyncGPU();
				refreshScreen();
				printf("Updated Line: %f, %f -> %f, %f\n",
					lines->Vtx()[pos * 2].x, lines->Vtx()[pos * 2].y,
					lines->Vtx()[pos * 2 + 1].x, lines->Vtx()[pos * 2 + 1].y);
			}
		}
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
		if (button == MOUSE_LEFT) {
			moving = false;
		}
		
	}

};
GreenTriangleApp app;