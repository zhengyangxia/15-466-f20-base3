#include "MazeMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "glm/fwd.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>


GLuint maze_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > maze_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("maze.pnct"));
	maze_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > maze_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("maze.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = maze_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = maze_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});


Scene::Transform* MazeMode::add_mesh_to_drawable(std::string mesh_name, glm::vec3 position) {
	Mesh const& mesh = maze_meshes->lookup(mesh_name);
	// create new transform
	scene.transforms.emplace_back();
	Scene::Transform* t = &scene.transforms.back();
	t->position = position;
	if (mesh_name == "Building") {
		t->position += glm::vec3(0, 0, 2);
		add_mesh_to_drawable("Building1", position-glm::vec3(0, -1, 0));
		add_mesh_to_drawable("Building2", position-glm::vec3(-1, 0, 0));
		add_mesh_to_drawable("Building1", position-glm::vec3(0, -1, 0));
		add_mesh_to_drawable("Building2", position-glm::vec3(1, 0, 0));
	} else if (mesh_name == "Building2") {
		t->rotation = glm::quat(0.7071f, 0.0f, 0.0f, 0.7071f);
	} else if (mesh_name == "water opossum ") {
		t->scale = glm::vec3(0.15, 0.15, 0.15);
		t->rotation = glm::quat(0.7071f, 0.0f, 0.0f, 0.7071f) * glm::quat(0.7071f, 0.7071f, 0.0f, 0.0f);
	} else if (mesh_name == "cake") {
		t->scale = glm::vec3(0.3, 0.3, 0.3);
		t->rotation = glm::quat(0.7071f, 0.7071f, 0.0f, 0.0f);
	}
	t->name = mesh_name;
	t->parent = nullptr;

	// create new drawable
	Scene::Drawable drawable(t);
	drawable.pipeline = lit_color_texture_program_pipeline;
	drawable.pipeline.vao = maze_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;
	scene.drawables.emplace_back(drawable);

	// return the transform
	return t;
}

void MazeMode::load_level(int level) {

	while (scene.drawables.size() > 0) {
		scene.drawables.pop_back();
	}

	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	load_png(data_path("../maze/" + std::to_string(level) + ".png"), &size, &data, UpperLeftOrigin);
	
	map = new char*[size.y];
	map_size = size;
	
	for (size_t i = 0; i < size.y; ++i) {
		map[i] = new char[size.x];
		for (size_t j = 0; j < size.x; ++j) {
			size_t x = 2*j;
			size_t y = 2*i;
			glm::u8vec4 c = data[i*size.x+j];
			if (c.r == 0 && c.g == 0 && c.b == 0) { 			// black -> wall
				map[i][j] = 'w';
				add_mesh_to_drawable("Building", glm::vec3(size.x*2-x, y, 0));
			} 
			else if (c.r == 255 && c.g == 0 && c.b == 0) {		// red -> target
				map[i][j] = 't';
				target = add_mesh_to_drawable("cake", glm::vec3(size.x*2-x, y, 0));
			} 
			else if (c.r == 0 && c.g == 0 && c.b == 255) {   	// blue -> player
				map[i][j] = 'f';
				player_pos = glm::ivec2(j, i);
				player = add_mesh_to_drawable("water opossum ", glm::vec3(size.x*2-x, y, 0));
				bar = add_mesh_to_drawable("Wall", glm::vec3(size.x*2-x, y+2, 5));
				bar_ref = add_mesh_to_drawable("Wall", glm::vec3(size.x*2-x, y+2, 5));
				bar->scale = glm::vec3(0.05f, 0.25f, 0.05f);
				bar_ref->scale = glm::vec3(0.05f, 0.25f, 0.05f);
				player_base_position = player->position;
				player_base_rotation = player->rotation;
			} else {
				map[i][j] = 'f';
			}
			
			// floor
			add_mesh_to_drawable("Floor", glm::vec3(size.x*2-x, y, 0));
		}
		
	}
	Sound::Sample const *maze_sample = new Sound::Sample(data_path(std::to_string(level)+".wav"));
	music_loop = Sound::loop_3D(*maze_sample, 1.0f, target->position, 5.0f);
	beat_interval = 60*sample_per_sec/bpm[level];
}



MazeMode::MazeMode() : scene(*maze_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	
	load_level(MazeMode::level);
	camera->transform->position = player->position + glm::vec3(0.0f, 12.0f, 12.0f);
	camera_base_position = camera->transform->position;
	

	
}

MazeMode::~MazeMode() {
}

bool MazeMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (moving) return false;
	if (evt.type == SDL_KEYDOWN) {
		hit = music_loop->i;
		bar->scale *= 1.5f;
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = true;
			moving = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = true;
			moving = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			moving = true;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			moving = true;
			down.pressed = true;
			return true;
		}
	}
	
	return false;
}

bool MazeMode::legal(glm::ivec2 new_pos, int energy){
	if (new_pos.x < 0 || new_pos.y < 0 || new_pos.y >= map_size.y || new_pos.x >= map_size.x) return false;
	if (map[new_pos.y][new_pos.x] == 'w' && energy <= 3){
		return false;
	}
	return true;
}

void MazeMode::end_move(){
	left.pressed = false;
	right.pressed = false;
	down.pressed = false;
	up.pressed = false;
	
	moving = false;
}

bool MazeMode::update(float elapsed) {

	float dis = ((music_loop->i-beat_offset[level])%beat_interval)/(float)beat_interval-0.5f;

	if (moving)
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 10.0f;
		glm::vec2 move = glm::vec2(0.0f);

		if (left.pressed) move.x = -1.0f;
		if (right.pressed) move.x = 1.0f;
		if (down.pressed) move.y = +1.0f;
		if (up.pressed) move.y = -1.0f;

		int new_dir;
		if (left.pressed) new_dir = -1;
		if (right.pressed) new_dir = 1;
		if (down.pressed) new_dir = 2;
		if (up.pressed) new_dir = 0;

		if (legal(player_pos+(glm::ivec2)move, energy)){
			dmov += PlayerSpeed*elapsed;
			dmov = std::fmin(dmov, 1.0f);
			player->position = player_base_position + dmov * (move.x * dirx + move.y * diry);
			player->rotation = glm::angleAxis(
						glm::radians(dmov*(new_dir-player_dir)*90),
						dirz
					) * player_base_rotation;
			camera->transform->position = camera_base_position + dmov * (move.x * dirx + move.y * diry);
			Sound::listener.set_position_right(camera->transform->position, dirx, 1.0f / 60.0f);
			if (dmov == 1.0f){
				dmov = 0;
				player_pos += (glm::ivec2)move;
				player_base_position = player->position;
				player_base_rotation = player->rotation;
				camera_base_position = camera->transform->position;
				player_dir = new_dir;
				end_move();
				if (energy <= 3 && map[player_pos.y][player_pos.x] == 'f')
					player_base_position.z = 0;
				if (map[player_pos.y][player_pos.x] == 't'){
					MazeMode::level ++;
					return true;
				}
			}
			

			if (hit > 0){
				if (std::abs(dis) <= beat_range && miss){
					miss = false;
					energy ++;
					if (energy > 3){
						player_base_position.z = 2;
					}
				} 
				if (std::abs(dis) > beat_range){
					miss = true;
				}
				hit = -1;
			}
				
		} else {
			end_move();
		}
	
	
	}
	if (dis > beat_range && miss)
		energy = 0;
	
	if (dis < -beat_range)
		miss = true;
	bar->scale = glm::vec3(0.05f, 0.25f, 0.05f);
	bar_ref->position = camera->transform->position + bar_offset;
	bar->position = bar_ref->position + dis * dirx * 4.0f;
	
	return false;
	
}
	

void MazeMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	
	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text(std::string("COMBO ")+std::to_string(energy)+"/4",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(std::string("COMBO ")+std::to_string(energy)+"/4",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H - ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text(std::string("Level ")+std::to_string(MazeMode::level+1),
			glm::vec3(-aspect + 0.1f * H, 0.9f-0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(std::string("Level ")+std::to_string(MazeMode::level+1),
			glm::vec3(-aspect + 0.1f * H + ofs, 0.9f-0.1f * H - ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
