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
	size_t get_next_byte_pos();
	char **map = nullptr;
	glm::ivec2 map_size;

	// status
	Scene::Transform *player = nullptr;
	Scene::Transform *target = nullptr;
	Scene::Transform *bar = nullptr;
	glm::vec3 bar_base_position;
	static int level;
	uint energy = 0;
	int hit = -1;

	int next_byte_pos = 0;
	int bpm = 60;
	int sample_per_sec = 48000;
	int beat_interval = 60*sample_per_sec/bpm;

	glm::vec3 dirx = glm::vec3(-2.0f, 0.0f, 0.0f);
	glm::vec3 diry = glm::vec3(0.0f, 2.0f, 0.0f);
	glm::vec3 dirz = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::ivec2 player_pos = glm::ivec2(0,0);
};
