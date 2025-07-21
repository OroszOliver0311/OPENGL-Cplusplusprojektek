//=============================================================================================
// Zöld háromszög: A framework.h osztályait felhasználó megoldás
//=============================================================================================
#include "framework.h"

// csúcspont árnyaló
const char * vertSource = R"(
	#version 330				
    precision highp float;

	uniform mat4 MVP; 	// Modell-Nézeti-Perspektív transzformáció
	layout(location = 0) in vec2 vp; // 0. bemeneti regiszter

	void main() {
   gl_Position = MVP * vec4(vp.xy, 0, 1); 	
	}
)";

// pixel árnyaló
const char * fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;			// konstans szín
	out vec4 fragmentColor;		// pixel szín

	void main() {
		fragmentColor = vec4(color, 1); // RGB -> RGBA
	}
)";

const int winWidth = 600, winHeight = 600;

class Camera {
	vec3 wCenter; 
	vec3 wSize;   

public:
	Camera(vec3 center, vec3 size) : wCenter(center), wSize(size) {}
	mat4 V() {return translate(-wCenter);}
	mat4 P() {return scale(vec3(1.0f / wSize.x, 1.0f / wSize.y, 1.0f));}
	//mat4 Vinv() { return inverse(V()); }
	//mat4 Pinv() {return inverse(P());}
	vec3 ScreenToWorld(vec3 screenPos) {
		float normalizedX = (2.0f * screenPos.x) / winWidth - 1.0f;
		float normalizedY = 1.0f - (2.0f * screenPos.y) / winHeight; 
		float normalizedZ = screenPos.z;
		//c4 worldPos = Vinv() * Pinv() * vec4(normalizedX, normalizedY, normalizedZ, 1.0f);
		//vec4 worldPos = Vinv() * vec4(screenPos.x,screenPos.y,screenPos.z, 1.0f);
		//worldPos = Vinv() * worldPos; 
		//return vec3(worldPos.x, worldPos.y, worldPos.z);
		return(vec3(normalizedX * 20, normalizedY * 20, normalizedZ * 20));
	}
};

Camera* camera;

class Primitive2D {
	unsigned int vao, vbo;	
protected:
	std::vector<vec3> vtx;	
public:
	Primitive2D() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}
	std::vector<vec3>& Vtx() { return vtx; }
	void Load() {
		glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
	}
	void Draw(GPUProgram* prog, int type, vec3 color,float angle, vec3 rotatevec, vec3 transaltevec) {
		if(vtx.size() == 0) return;
		
		mat4 M = scale(vec3(1.0f, 1.0f, 1.0f)) * rotate(angle,rotatevec) * translate(transaltevec);
		mat4 MVP = M * camera->V() * camera->P();
		
		prog->setUniform(MVP, "MVP");
		prog->setUniform(color, "color");
			glBindVertexArray(vao);
			glDrawArrays(type, 0, (int)vtx.size());
		
	}
  ~Primitive2D() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

class Spline {
public:
	Primitive2D ControlPoints;
	Primitive2D SplinePoints;
	std::vector<float> ts;
	Spline() : ControlPoints(), SplinePoints() {}
	void AddControlPoint(vec3 mouse) {
		float ti = ControlPoints.Vtx().size(); 
		ControlPoints.Vtx().push_back(mouse); 
		ts.push_back(ti); 
		SplinePoints.Vtx().clear(); 
	}
	
	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		float dt = t1 - t0;
		t = (t - t0) / dt;
		vec3 a0 = p0;
		vec3 a1 = v0 * dt;
		vec3 a2 = (p1 - p0) * 3.0f - (v1 + v0 * 2.0f) * dt;
		vec3 a3 = (p0 - p1) * 2.0f + (v1 + v0) * dt;
		return ((a3 * t + a2) * t + a1) * t + a0;
	}
	vec3 Hermitet(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		float dt = t1 - t0;
		t = (t - t0) / dt;
		vec3 a0 = p0;
		vec3 a1 = v0 * dt;
		vec3 a2 = (p1 - p0) * 3.0f - (v1 + v0 * 2.0f) * dt;
		vec3 a3 = (p0 - p1) * 2.0f + (v1 + v0) * dt;
		return (3.0f * a3 * t + 2.0f * a2) * t + a1;
	}
	vec3 Hermitett(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		float dt = t1 - t0;
		t = (t - t0) / dt;
		vec3 a0 = p0;
		vec3 a1 = v0 * dt;
		vec3 a2 = (p1 - p0) * 3.0f - (v1 + v0 * 2.0f) * dt;
		vec3 a3 = (p0 - p1) * 2.0f + (v1 + v0) * dt;
		return 6.0f * a3 * t + 2.0f * a2;
	}

	vec3 r(float t) {
		if (ControlPoints.Vtx().size() < 2) return ControlPoints.Vtx().back(); 

		for (unsigned int i = 0; i < ControlPoints.Vtx().size() - 1; i++) {
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec3  p0 = ControlPoints.Vtx()[i];
				if (i > 0) p0 =  ControlPoints.Vtx()[i - 1] ;
				vec3 p1 = ControlPoints.Vtx()[i]; 
				vec3 p2 = ControlPoints.Vtx()[i+1];  
				vec3  p3 = ControlPoints.Vtx()[i+1];
				if(i < ControlPoints.Vtx().size() - 2) p3 = ControlPoints.Vtx()[i + 2];
				vec3 v0 = (p2 - p0) * 0.5f; 
				vec3 v1 = (p3 - p1) * 0.5f; 
				return Hermite(p1, v0, ts[i], p2, v1, ts[i + 1], t);
			}
		}
		return ControlPoints.Vtx().back(); 
	}
	vec3 rt(float t) {
		if (ControlPoints.Vtx().size() < 2) return ControlPoints.Vtx().back();

		for (unsigned int i = 0; i < ControlPoints.Vtx().size() - 1; i++) {
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec3  p0 = ControlPoints.Vtx()[i];
				if (i > 0) p0 = ControlPoints.Vtx()[i - 1];
				vec3 p1 = ControlPoints.Vtx()[i];
				vec3 p2 = ControlPoints.Vtx()[i + 1];
				vec3  p3 = ControlPoints.Vtx()[i + 1];
				if (i < ControlPoints.Vtx().size() - 2) p3 = ControlPoints.Vtx()[i + 2];
				vec3 v0 = (p2 - p0) * 0.5f;
				vec3 v1 = (p3 - p1) * 0.5f;
				return Hermitet(p1, v0, ts[i], p2, v1, ts[i + 1], t);
			}
		}
		return ControlPoints.Vtx().back();
	}
	vec3 rtt(float t) {
		if (ControlPoints.Vtx().size() < 2) return ControlPoints.Vtx().back();

		for (unsigned int i = 0; i < ControlPoints.Vtx().size() - 1; i++) {
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec3  p0 = ControlPoints.Vtx()[i];
				if (i > 0) p0 = ControlPoints.Vtx()[i - 1];
				vec3 p1 = ControlPoints.Vtx()[i];
				vec3 p2 = ControlPoints.Vtx()[i + 1];
				vec3  p3 = ControlPoints.Vtx()[i + 1];
				if (i < ControlPoints.Vtx().size() - 2) p3 = ControlPoints.Vtx()[i + 2];
				vec3 v0 = (p2 - p0) * 0.5f;
				vec3 v1 = (p3 - p1) * 0.5f;
				return Hermitett(p1, v0, ts[i], p2, v1, ts[i + 1], t);
			}
		}
		return ControlPoints.Vtx().back();
	}
	
	
	
	
	
	
	
	
	
	
	void AddSpline() {
		if (ControlPoints.Vtx().size() < 2) return;

		for (int i = 0; i <= 100; i++) {
			float t = ts[0] + (ts[ts.size() - 1] - ts[0]) * (i / 100.0f);
			SplinePoints.Vtx().push_back(r(t));
		}
	}
	
	void Draw(GPUProgram* prog) {
		if (ControlPoints.Vtx().size() == 0) return;
		if (ControlPoints.Vtx().size() > 1) {
			SplinePoints.Load();
			glLineWidth(3.0f);
			SplinePoints.Draw(prog, GL_LINE_STRIP, vec3(1.0f, 1.0f, 0.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f));
		}
		glPointSize(10.0f);
		ControlPoints.Load();
		ControlPoints.Draw(prog, GL_POINTS, vec3(1.0f, 0.0f, 0.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f));
	
	}


};
class Gondola {
public:
	int State; // 0 = idle, 1 = mozgás, 2 = lerepült
	Primitive2D wheelBody;
	Primitive2D wheelOutline;
	Primitive2D spokes;
	Spline* track;
	vec3 prevpos;
	vec3 position;
	vec3 cposition;
	vec3 T;
	vec3 N;
	float tau;
	float v;
	float elfordulas;
	float szogsebesseg;
	float szoggyorsulas;
	float tehetlennyomatek = 0.0f;
	vec3 gorbulet;
	vec3 K;
	const float g = 40.0f; 

	Gondola(Spline* pTrack) {
		track = pTrack;
		State = 0; 
	}

	void Start() {
		if (State == 0 && track->ControlPoints.Vtx().size() >= 2) {
			State = 1; 
			tau = 0.01f;
			v = 0.0f;
			position = track->r(tau);
			T = track->rt(tau) / length(track->rt(tau));
			N.x = -T.y; N.y = T.x; N.z = 0.0f;
			cposition = position + N * 2.0f;
			szogsebesseg = 0.0f;
			elfordulas = 0.0f;

		}
	}
	void Animate(float dt) {
		gorbulet = (track->rtt(tau) * N) / (length(track->r(tau)) * length(track->r(tau)));
		K = (g * N + v * v * gorbulet);
		if (length(K) >= 0) {
			v = sqrt(2 * g * (track->ControlPoints.Vtx()[0].y - track->r(tau).y));
			float Dtau = v * dt / length(track->rt(tau));
			tau += Dtau;
			position = track->r(tau);
			T = track->rt(tau) / length(track->rt(tau));
			N.x = -T.y; N.y = T.x; N.z = 0.0f;
			cposition = position + N * 2.0f;
			szogsebesseg = v / length(track->rt(tau));
			szoggyorsulas = length(track->rtt(tau)) / length(track->rt(tau));
			elfordulas += szogsebesseg * Dtau + 0.5f * szoggyorsulas * Dtau * Dtau;

		}
		if (length(K) < 0) {
		}
	
	
	
	}

	void Draw(GPUProgram* prog) {
		if (position.x == 0.0f && position.y == 0.0f) return;

		wheelBody.Vtx().clear();
		wheelOutline.Vtx().clear();
		spokes.Vtx().clear();

		
		for (int i = 0; i < 32; i++) { 
			float theta = 2.0f * M_PI * float(i) / 32.0f;
			float x = 2.0f *  cos(theta - elfordulas);
			float y = 2.0f * sin(theta - elfordulas);
			wheelOutline.Vtx().push_back(cposition + vec3(x, y, 0));
			wheelBody.Vtx().push_back(cposition + vec3(x, y, 0));
		}


		for (int i = 0; i < 4; i++) { 
			float theta = 2.0f * M_PI * float(i) / 4.0f;
			float x = 2.0f * cos(theta - elfordulas);
			float y = 2.0f *  sin(theta - elfordulas);
			spokes.Vtx().push_back(cposition);
			spokes.Vtx().push_back(cposition + vec3(x, y, 0));
		}

	
		wheelBody.Load();
		wheelOutline.Load();
		spokes.Load();

		glLineWidth(3.0f);
		wheelBody.Draw(prog, GL_TRIANGLE_FAN, vec3(1.0f, 1.0f, 1.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f));
		wheelOutline.Draw(prog, GL_LINE_LOOP, vec3(1.0f, 1.0f, 1.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f));
		spokes.Draw(prog, GL_LINES, vec3(1.0f, 1.0f, 1.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f));
	}
};




	class GreenTriangleApp : public glApp {
	public:
		GPUProgram* gpuProgram;	   // csúcspont és pixel árnyalók
		Primitive2D* triangle;
		Gondola* gondola;
		float t;
		bool moving;
		Spline* CattMullSpline;
		GreenTriangleApp() : glApp("Green triangle") {}

		// Inicializáció, 
		void onInitialization() {
			camera = new Camera(vec3(0.0f, 0.0f, 0.0f), vec3(20.0f, 20.0f, 1.0f));
			t = 0.01;
			moving = false;
			CattMullSpline = new Spline();
			gondola = new Gondola(CattMullSpline);
			gpuProgram = new GPUProgram(vertSource, fragSource);

		}

		// Ablak újrarajzolás
		void onDisplay() {
			glClearColor(0, 0, 0, 0);     // háttér szín
			glClear(GL_COLOR_BUFFER_BIT); // rasztertár törlés
			glViewport(0, 0, winWidth, winHeight);
			gondola->Draw(gpuProgram);
			CattMullSpline->Draw(gpuProgram);
		
		}
		void onMousePressed(MouseButton but, int pX, int pY) {
			//vec3 mouse = vec3((2.0f * pX) / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight, 1.0f);
			//printf("%d,%d\n", pX, pY);
			//printf("%f,%f\n", mouse.x, mouse.y);
			//printf("%f %f\n", camera->ScreenToWorld(vec3(pX, pY, 1.0f)).x, camera->ScreenToWorld(vec3(pX, pY, 1.0f)).y);
			//CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(pX, pY, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(50, 85, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(163, 503, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(408, 501, 1.0f)));
		CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(344, 311, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(218, 376, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(286, 478, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(451, 440, 1.0f)));
			CattMullSpline->AddControlPoint(camera->ScreenToWorld(vec3(557, 55, 1.0f)));
			CattMullSpline->AddSpline();
			refreshScreen();
		}
		void onKeyboard(int key) {
			if (key == ' ') { 
				gondola->Start();
				gondola->Draw(gpuProgram);
				refreshScreen();
				moving = true;
			}
		}
		void onTimeElapsed(float startTime, float endTime) {
			if (moving) {
				const float dt = fabs(startTime- endTime);
				gondola->Animate(dt);
				refreshScreen();
			}
		}

};

GreenTriangleApp app;

