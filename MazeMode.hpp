#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct MazeMode : Mode {
	MazeMode();
	virtual ~MazeMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual bool update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void end_move();

	//----- game state -----
	bool legal(glm::ivec2 new_pos, uint energy);

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	std::shared_ptr< Sound::PlayingSample > music_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

	// util
	Scene::Transform* add_mesh_to_drawable(std::string mesh_name, glm::vec3 position);
	void load_level(int level);
	char **map = nullptr;
	glm::ivec2 map_size;

	// status
	Scene::Transform *player = nullptr;
	Scene::Transform *target = nullptr;
	Scene::Transform *bar = nullptr;
	Scene::Transform *bar_ref = nullptr;
	glm::vec3 bar_offset = glm::vec3(0.0f, -3.0f, -2.0f);
	glm::quat player_base_rotation;
	glm::vec3 player_base_position;
	glm::vec3 camera_base_position;
	float dmov = 0.0f;
	static int level;
	uint energy = 0;
	int hit = -1;
	bool moving = false;
	bool miss = false;
	int player_dir = 0;
	int bpm[3] = {120, 125, 105};
	int beat_offset[3] = {40, 40, 56304};
	int sample_per_sec = 48000;
	int beat_interval;
	float beat_range = 0.15f;

	glm::vec3 dirx = glm::vec3(-2.0f, 0.0f, 0.0f);
	glm::vec3 diry = glm::vec3(0.0f, 2.0f, 0.0f);
	glm::vec3 dirz = glm::vec3(0.0f, 0.0f, -1.0f);

	glm::ivec2 player_pos = glm::ivec2(0,0);
};
