//=============================================================================================
// Zöld háromszög: A framework.h osztályait felhasználó megoldás
//=============================================================================================
#include "framework.h"

// csúcspont árnyaló
const char* vertSource = R"(
	#version 330
    precision highp float;

 
	layout(location = 0) in vec2 vertexXY;
	layout(location = 1) in vec2 vertexUV; 
	
	out vec2 texCoord;					

	void main() {
		texCoord = vertexUV;		
		gl_Position = vec4(vertexXY, 0, 1); 		
	}
)";

// pixel árnyaló
const char* fragSource = R"(
#version 330
precision highp float;

 bool isNight = false;
uniform bool useTexture;
uniform vec3 color;
uniform sampler2D textureUnit;
uniform float time; 
uniform float dayOfYear; 

in vec2 texCoord;
out vec4 outColor;

const float axialTilt = 23.0; 
const float PI = 3.141592653589793;

void main() {
vec4 originalColor;
if(useTexture) {
originalColor =texture(textureUnit, texCoord); isNight = true;} 
else {originalColor = vec4(color, 1.0);}

    float longitude = (texCoord.x * 2.0 - 1.0) * 180.0; 
    float latitude = (1.0 - texCoord.y * 2.0) * 90.0; 

    float solarDeclination = axialTilt * sin(2.0 * PI * (dayOfYear - 80.0) / 365.0);

    float hourAngle = radians((time - 12.0) * 15.0); 

    float cosZenithAngle = sin(radians(solarDeclination)) * sin(radians(latitude)) +
                         cos(radians(solarDeclination)) * cos(radians(latitude)) *
                         cos(hourAngle - radians(longitude));

    if(cosZenithAngle <= 0.0 && isNight) { 
		originalColor.rgb *= 0.5;
    }
    
    outColor = originalColor;
}
)";
const int winWidth = 600, winHeight = 600;
vec2 ScreenToNDC(vec2 screen) {
	return vec2(2.0f * screen.x / winWidth - 1.0f, 1.0f - 2.0f * screen.y / winHeight);
}

vec2 MercatorToMap(const vec2& mercator) {
	float lon = mercator.x * M_PI;
	float lat = 2.0f * atan(exp(mercator.y * M_PI)) - M_PI / 2.0f;
	return vec2(lon,lat);
}
vec3 MapToSphere(const vec2& map) {
	float x = cos(map.y) * cos(map.x);
	float y = cos(map.y) * sin(map.x);
	float z = sin(map.y);
	return normalize(vec3(x, y, z));
}
vec2 SphereToMap(const vec3& sphere) {
	float lon = atan2(sphere.y, sphere.x);
	float lat = asin(sphere.z);
	return vec2(lon, lat);
}
vec2 MapToMercator(const vec2& map) {
	float x = map.x / M_PI;
	float y = log(tan(M_PI / 4.0f + map.y / 2.0f)) / M_PI;
	return vec2(x, y);
}
class Texture2 {
	unsigned int textureId = 0;
public:

	Texture2(int width, int height, std::vector<vec3>& image) {
		glGenTextures(1, &textureId); 
		glBindTexture(GL_TEXTURE_2D, textureId);    
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); 
		glBindTexture(GL_TEXTURE_2D, textureId); 
	}
	~Texture2() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};
class Map {
	const std::vector<unsigned char> image = {
	252, 252, 252, 252, 252, 252, 252, 252, 252, 0, 9, 80, 1, 148, 13, 72, 13, 140, 25, 60, 21, 132, 41, 12, 1, 28,
	25, 128, 61, 0, 17, 4, 29, 124, 81, 8, 37, 116, 89, 0, 69, 16, 5, 48, 97, 0, 77, 0, 25, 8, 1, 8, 253, 253, 253, 253,
	101, 10, 237, 14, 237, 14, 241, 10, 141, 2, 93, 14, 121, 2, 5, 6, 93, 14, 49, 6, 57, 26, 89, 18, 41, 10, 57, 26,
	89, 18, 41, 14, 1, 2, 45, 26, 89, 26, 33, 18, 57, 14, 93, 26, 33, 18, 57, 10, 93, 18, 5, 2, 33, 18, 41, 2, 5, 2, 5, 6,
	89, 22, 29, 2, 1, 22, 37, 2, 1, 6, 1, 2, 97, 22, 29, 38, 45, 2, 97, 10, 1, 2, 37, 42, 17, 2, 13, 2, 5, 2, 89, 10, 49,
	46, 25, 10, 101, 2, 5, 6, 37, 50, 9, 30, 89, 10, 9, 2, 37, 50, 5, 38, 81, 26, 45, 22, 17, 54, 77, 30, 41, 22, 17, 58,
	1, 2, 61, 38, 65, 2, 9, 58, 69, 46, 37, 6, 1, 10, 9, 62, 65, 38, 5, 2, 33, 102, 57, 54, 33, 102, 57, 30, 1, 14, 33, 2,
	9, 86, 9, 2, 21, 6, 13, 26, 5, 6, 53, 94, 29, 26, 1, 22, 29, 0, 29, 98, 5, 14, 9, 46, 1, 2, 5, 6, 5, 2, 0, 13, 0, 13,
	118, 1, 2, 1, 42, 1, 4, 5, 6, 5, 2, 4, 33, 78, 1, 6, 1, 6, 1, 10, 5, 34, 1, 20, 2, 9, 2, 12, 25, 14, 5, 30, 1, 54, 13, 6,
	9, 2, 1, 32, 13, 8, 37, 2, 13, 2, 1, 70, 49, 28, 13, 16, 53, 2, 1, 46, 1, 2, 1, 2, 53, 28, 17, 16, 57, 14, 1, 18, 1, 14,
	1, 2, 57, 24, 13, 20, 57, 0, 2, 1, 2, 17, 0, 17, 2, 61, 0, 5, 16, 1, 28, 25, 0, 41, 2, 117, 56, 25, 0, 33, 2, 1, 2, 117,
	52, 201, 48, 77, 0, 121, 40, 1, 0, 205, 8, 1, 0, 1, 12, 213, 4, 13, 12, 253, 253, 253, 141 };
	std::vector<vec3> background = makeBackground(image);
	std::vector<vec3> makeBackground(const std::vector<unsigned char>& image) {
		std::vector<vec3> background(64 * 64); 
		unsigned int pixelIndex = 0;

		for (const auto& byte : image) {
			unsigned char H = (byte >> 2) & 0x3F; 
			unsigned char I = byte & 0x03;        
			vec3 color(0.0f); 
			switch (I) {
			case 0: color = vec3(1.0f, 1.0f, 1.0f); break; 
			case 1: color = vec3(0.0f, 0.0f, 1.0f); break; 
			case 2: color = vec3(0.0f, 1.0f, 0.0f); break; 
			case 3: color = vec3(0.0f, 0.0f, 0.0f); break; 
			}
			for (unsigned char j = 0; j < H + 1 && pixelIndex < 64 * 64; ++j) {
				unsigned int row = pixelIndex / 64; 
				unsigned int col = pixelIndex % 64; 

				unsigned int index = row * 64 + col; 
				background[index] = color;       
				++pixelIndex;
			}
		}
		return background;
	}
	unsigned int vao = 0, vbo[2];
	std::vector<vec2> vtx = { vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(-1.0f, 1.0f) };
	Texture2 texture = Texture2(64, 64, background);
public:
	Map() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, &vbo[0]); 
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0); 
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL); 
		
		std::vector<vec2> uvs = { vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f) };

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); 
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1); 
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL); 
	}
	void Draw(GPUProgram* gpuProgram) {
		int textureUnit = 0; 
		gpuProgram->setUniform(true, "useTexture");
		gpuProgram->setUniform(textureUnit, "textureUnit"); 
		texture.Bind(textureUnit);                          
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);        
	}
	virtual ~Map() {
		glDeleteBuffers(2, vbo);      
		glDeleteVertexArrays(1, &vao);
	}
};
class Station : public Geometry<vec2> {
public:
	void addStation(const vec2& position) {
		vtx.push_back(position);
		updateGPU();
	}
	void drawStation(GPUProgram* gpuProgram) {
		if (this->Vtx().size() == 0) return;
		gpuProgram->setUniform(false, "useTexture");
		glPointSize(10.0f);
		Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
	}


};
class Path : public Geometry<vec2> {
public:
	std::vector<vec2> stations;
	void addStation(const vec2& station) {
		stations.push_back(station);
		vtx.clear();
	}
	void MakePath() {
		if (stations.size() < 2) return;
		for (size_t i = 0; i < stations.size() - 1; i++) {

			vec3 start = MapToSphere(MercatorToMap(stations[i]));
			vec3 end = MapToSphere(MercatorToMap(stations[i+1]));
		
			float D = acos(dot(start, end));
			for (float t = 0.00f; t < 1.0f; t += 0.01f) {
				vec3 spherePoint =  sin((1 - t) * D) * start / sin(D) + (sin(t * D) * end / sin(D));
				vec2 mercatorPoint = MapToMercator(SphereToMap(spherePoint));
				vtx.push_back(mercatorPoint);
			}
		
		}
	
	}
	void drawPath(GPUProgram* gpuProgram) {
		if (this->Vtx().size() == 0) return;
		updateGPU();
		gpuProgram->setUniform(false, "useTexture");
		glLineWidth(3.0f);
		Draw(gpuProgram, GL_LINE_STRIP, vec3(1.0f, 1.0f, 0.0f));
	}
};



class GreenTriangleApp : public glApp {

	GPUProgram* gpuProgram;	   
	Map* map;
	Station* station;
	Path* path;
	float currentHour = 0.0f;
	float dayOfYear = 172.0f;
public:
	GreenTriangleApp() : glApp("Green triangle") { }

	// Inicializáció, 
	void onInitialization() {
		map = new Map();
		station = new Station();
		path = new Path();
		gpuProgram = new GPUProgram(vertSource, fragSource);
	}

	// Ablak újrarajzolás
	void onDisplay() {
		glClearColor(0, 0, 0, 0);     // háttér szín
		glClear(GL_COLOR_BUFFER_BIT); // rasztertár törlés
		glViewport(0, 0, winWidth, winHeight);
		map->Draw(gpuProgram);
		path->drawPath(gpuProgram);
		station->drawStation(gpuProgram);
	}
	void onMousePressed(MouseButton but, int pX, int pY) {
		vec2 normalPos = ScreenToNDC(vec2(pX, pY));
		station->addStation(normalPos);
		//printf("x: %f, y: %f\n", normalPos.x, normalPos.y);
		/*vec2 normalPos = vec2(-0.65f, 0.33f);
		printf("x: %f, y: %f\n", normalPos.x, normalPos.y);
		vec2 mercatorPos = NormalToMercator(normalPos);
		printf("x: %f, y: %f\n", mercatorPos.x, mercatorPos.y);
		vec2 mapPos = MercatorToMap(mercatorPos);
		printf("x: %f, y: %f\n", mapPos.x, mapPos.y);
		vec3 spherePos = MapToSphere(mapPos);
		printf("x: %f, y: %f, z: %f\n", spherePos.x, spherePos.y, spherePos.z);
		vec2 worldPos = SphereToWorld(spherePos);
		printf("x: %f, y: %f\n", worldPos.x, worldPos.y);
		vec2 mercatorPos2 = WorldToMercator(worldPos);
		printf("x: %f, y: %f\n", mercatorPos2.x, mercatorPos2.y);
		vec2 normalPos2 = MercatorToNormal(mercatorPos2);
		printf("x: %f, y: %f\n", normalPos2.x, normalPos2.y); */
		/*station->addStation(vec2(-0.65f, 0.33f));
		path->addStation(vec2(-0.65f, 0.33f));
		station->addStation(vec2(0.19f, 0.41f));
		path->addStation(vec2(0.19f, 0.41f));
		station->addStation(vec2(0.195f, 0.415f));
		path->addStation(vec2(0.195f, 0.415f));
		station->addStation(vec2(0.69f, 0.32f));
		path->addStation(vec2(0.69f, 0.32f));
		station->addStation(vec2(-0.28f, -0.06f));
		path->addStation(vec2(-0.28f, -0.06f));
		station->addStation(vec2(-0.65f, 0.33f));
		path->addStation(vec2(-0.65f, 0.33f));*/
		path->addStation(normalPos);
		path->MakePath();
		refreshScreen();
	
	}
	void keyboard(unsigned char key, int x, int y) {
		if (key == 'n') {
			currentHour++;
			gpuProgram->setUniform(currentHour, "time");
			refreshScreen();
		}
	}

};

GreenTriangleApp app;

