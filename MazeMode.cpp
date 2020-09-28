#include "MazeMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
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

Load< Sound::Sample > maze_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("digital-lemonade-by-kevin-macleod-from-filmmusic-io.wav"));//data_path("dusty-floor.opus"));
});


Scene::Transform* MazeMode::add_mesh_to_drawable(std::string mesh_name, glm::vec3 position) {
	Mesh const& mesh = maze_meshes->lookup(mesh_name);
	// create new transform
	scene.transforms.emplace_back();
	Scene::Transform* t = &scene.transforms.back();
	t->position = position;
	// if (mesh_name == "Wall") {
	// 	t->scale.z = 2.0;
	// }
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
	for (size_t i = 0; i < size.y; ++i) {
		for (size_t j = 0; j < size.x; ++j) {
			size_t x = 2*j;
			size_t y = 2*i;
			glm::u8vec4 c = data[i*size.x+j];
			if (c.r == 0 && c.g == 0 && c.b == 0) { 			// black -> wall
				std::cout << "w ";
				add_mesh_to_drawable("Wall", glm::vec3(x, y, 0));
			} 
			else if (c.r == 255 && c.g == 0 && c.b == 0) {		// red -> target
				std::cout << "t ";
				target = add_mesh_to_drawable("Target", glm::vec3(x, y, 0));
			} 
			else if (c.r == 0 && c.g == 0 && c.b == 255) {   	// blue -> player
				std::cout << "p ";
				player = add_mesh_to_drawable("Player", glm::vec3(x, y, 0));
			} else {
				std::cout << "  ";
			}
			
			// floor
			if ((i+j)%2 == 0)
				add_mesh_to_drawable("Plane1", glm::vec3(x, y, -1));
			else
				add_mesh_to_drawable("Plane2", glm::vec3(x, y, -1));
		}
		std::cout << std::endl;
	}
	camera = &scene.cameras.front();
	camera->transform->position = player->position + glm::vec3(0.0f, 8.0f, 8.0f);
	
}


size_t MazeMode::get_next_byte_pos() {
	return next_byte_pos + (size_t) (60.0 / bpm * sample_per_sec);
}


MazeMode::MazeMode() : scene(*maze_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	load_level(level);

	//start music loop playing:
	// (note: position will be over-ridden in update())
	music_loop = Sound::loop_3D(*maze_sample, 1.0f, target->position, 10.0f);

	
}

MazeMode::~MazeMode() {
}

bool MazeMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		hit = music_loop->i;
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			return true;
		}
	}
	
	return false;
}

void MazeMode::update(float elapsed) {

	//move camera:
	{

		//combine inputs into a move:
		// constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		move.x -= left.downs;
		move.x += right.downs;
		move.y -= down.downs;
		move.y += up.downs;

		//make it so that moving diagonally doesn't go faster:
		// if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		// glm::mat4x3 frame = camera->transform->make_local_to_parent();
		// glm::vec3 right = frame[0];
		// //glm::vec3 up = frame[1];
		// glm::vec3 forward = -frame[2];
		uint dis = hit % beat_interval;
		
		if (hit > 0){
			if (dis <= 5000 || beat_interval-dis <= 5000){
				energy ++;
				if (energy > 3){
					player->position.z = 2;
				}
			} else {
				player->position.z = 0;
				energy = 0;
			}
		}
		
		player->position += move.x * dirx + move.y * diry;
		camera->transform->position += move.x * dirx + move.y * diry;
		// std::cout << camera->transform->position.x << " " << camera->transform->position.y << " " << camera->transform->position.z << " " << std::endl;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	hit = -1;
	std::cout << music_loop->i << std::endl;

	
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
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
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
		lines.draw_text(std::string("Combo Count:")+std::to_string(energy),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(std::string("Combo Count:")+std::to_string(energy),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
