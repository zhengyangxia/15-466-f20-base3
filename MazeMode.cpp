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

// Load< Sound::Sample > maze_sample(LoadTagDefault, []() -> Sound::Sample const * {
// 	return new Sound::Sample(data_path("digital-lemonade-by-kevin-macleod-from-filmmusic-io.wav"));//data_path("dusty-floor.opus"));
// });


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
	std::cout << "loading level " << level << std::endl; 
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
				std::cout << "w ";
				map[i][j] = 'w';
				add_mesh_to_drawable("Building", glm::vec3(size.x*2-x, y, 0));
			} 
			else if (c.r == 255 && c.g == 0 && c.b == 0) {		// red -> target
				std::cout << "t ";
				map[i][j] = 't';
				target = add_mesh_to_drawable("cake", glm::vec3(size.x*2-x, y, 0));
			} 
			else if (c.r == 0 && c.g == 0 && c.b == 255) {   	// blue -> player
				std::cout << "p ";
				map[i][j] = 'f';
				player_pos = glm::ivec2(j, i);
				player = add_mesh_to_drawable("water opossum ", glm::vec3(size.x*2-x, y, 0));
				bar = add_mesh_to_drawable("Wall", glm::vec3(size.x*2-x, y+2, 5));
				bar_ref = add_mesh_to_drawable("Wall", glm::vec3(size.x*2-x, y+2, 5));
				bar->scale = glm::vec3(0.5f, 0.1f, 0.1f);
				bar_ref->scale = glm::vec3(0.5f, 0.1f, 0.1f);
				bar_base_position = bar->position;
				
			} else {
				std::cout << "  ";
				map[i][j] = 'f';
			}
			
			// floor
			add_mesh_to_drawable("Floor", glm::vec3(size.x*2-x, y, 0));
		}
		std::cout << std::endl;
	}
	Sound::Sample const *maze_sample = new Sound::Sample(data_path(std::to_string(level)+".wav"));
	music_loop = Sound::loop_3D(*maze_sample, 1.0f, target->position, 5.0f);
}


size_t MazeMode::get_next_byte_pos() {
	return next_byte_pos + (size_t) (60.0 / bpm * sample_per_sec);
}


MazeMode::MazeMode() : scene(*maze_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	
	load_level(MazeMode::level);
	camera->transform->position = player->position + glm::vec3(0.0f, 12.0f, 12.0f);
	//start music loop playing:
	// (note: position will be over-ridden in update())
	

	
}

MazeMode::~MazeMode() {
}

bool MazeMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		hit = music_loop->i;
		bar->scale = glm::vec3(0.55f, 0.11f, 0.11f);
		if (evt.key.keysym.sym == SDLK_a) {
			player->rotation = glm::quat(0.7071f, 0.0f, 0.0f, 0.7071f) * glm::quat(0.7071f, 0.0f, 0.7071f, 0.0f) * glm::quat(0.7071f, 0.0f, 0.0f, 0.7071f);
			left.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			player->rotation = glm::quat(0.7071f, 0.7071f, 0.0f, 0.0f);
			right.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			player->rotation = glm::quat(0.7071f, 0.0f, 0.0f, 0.7071f) * glm::quat(0.7071f, 0.7071f, 0.0f, 0.0f);
			up.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			player->rotation = glm::quat(0.7071f, 0.0f, 0.0f, -0.7071f) * glm::quat(0.7071f, 0.7071f, 0.0f, 0.0f);
			down.downs += 1;
			return true;
		}
	}
	
	return false;
}

bool MazeMode::legal(glm::ivec2 new_pos, uint energy){
	// std::cout << new_pos.x << " " << new_pos.y << std::endl;
	if (new_pos.x < 0 || new_pos.y < 0 || new_pos.y >= map_size.y || new_pos.x >= map_size.x) return false;
	if (map[new_pos.y][new_pos.x] == 'w' && energy <= 3){
		// std::cout << map[new_pos.y][new_pos.x] << std::endl;
		return false;
	}
	return true;
}

bool MazeMode::update(float elapsed) {

	//move camera:
	{

		//combine inputs into a move:
		// constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::ivec2(0.0f);
		move.x -= left.downs;
		move.x += right.downs;
		move.y += down.downs;
		move.y -= up.downs;

		//make it so that moving diagonally doesn't go faster:
		// if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		// glm::mat4x3 frame = camera->transform->make_local_to_parent();
		// glm::vec3 right = frame[0];
		// //glm::vec3 up = frame[1];
		// glm::vec3 forward = -frame[2];
		
		// std::cout << music_loop->i << std::endl;
		float dis = ((music_loop->i)%beat_interval)/(float)beat_interval-0.5f;
		bar->position = bar_base_position + dis * dirx * 4.0f;
		bar_ref->position = bar_base_position;

		if (hit > 0 && legal(player_pos+(glm::ivec2)move, energy)){
			// bar->position = bar_base_position;
			
			std::cout << hit << " " << dis << std::endl;
			if (dis <= 0.1){
				energy ++;
				if (energy > 3){
					player->position.z = 2;
				}
			} else {
				if (map[player_pos.y][player_pos.x] == 'f'){
					player->position.z = 0;
				}
				energy = 0;
			}
		// std::cout << player->position.x << " " << player->position.y << std::endl;
			player_pos += (glm::ivec2)move;
			player->position += move.x * dirx + move.y * diry;
			bar_base_position += move.x * dirx + move.y * diry;
			camera->transform->position += move.x * dirx + move.y * diry;
			if (map[player_pos.y][player_pos.x] == 't'){
				MazeMode::level ++;
				return true;
			}
			Sound::listener.set_position_right(camera->transform->position, dirx, 1.0f / 60.0f);
		}
		
		
		// std::cout << camera->transform->position.x << " " << camera->transform->position.y << " " << camera->transform->position.z << " " << std::endl;
	}


	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	hit = -1;
	bar->scale = glm::vec3(0.5f, 0.1f, 0.1f);
	return false;
	// std::cout << music_loop->i << std::endl;
	
}
	

void MazeMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	
	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
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
		lines.draw_text(std::string("Combo Count:")+std::to_string(energy)+"/4",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(std::string("Combo Count:")+std::to_string(energy)+"/4",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H - ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text(std::string("Level: ")+std::to_string(MazeMode::level+1),
			glm::vec3(-aspect + 0.1f * H, 0.9f-0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(std::string("Level: ")+std::to_string(MazeMode::level+1),
			glm::vec3(-aspect + 0.1f * H + ofs, 0.9f-0.1f * H - ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
