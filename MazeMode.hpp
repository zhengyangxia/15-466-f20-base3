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
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	

	//----- game state -----
	bool legal(glm::ivec2 new_pos);

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
	glm::uvec2 map_size;

	// status
	Scene::Transform *player = nullptr;
	Scene::Transform *target = nullptr;
	int level = 0;
	uint energy = 0;
	int hit = -1;

	size_t next_byte_pos = 0;
	size_t bpm = 120;
	size_t sample_per_sec = 48000;
	size_t beat_interval = 60.0f/bpm*sample_per_sec;

	glm::ivec3 dirx = glm::ivec3(-2, 0, 0);
	glm::ivec3 diry = glm::ivec3(0, 2, 0);
	glm::ivec3 dirz = glm::ivec3(0, 0, 1);

	glm::ivec2 player_pos = glm::ivec2(0,0);
};
