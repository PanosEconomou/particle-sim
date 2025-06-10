// === main.cpp ===
#include "vulkan_context.h"
#include "compute_pipeline.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>


// Create the struct for the particles
struct Particle {
	float position[2];
	float velocity[2];
};

// Create a struct for the parameters to pass to the shader
struct Param {
	uint32_t num_particles;
	float pad;
	float bounds[2];
};

// Some cool global constants
const int FRAMES 		= 100000;
const float VELMAG 		= 5;


void printParticlesTerminal(const std::vector<Particle>& particles, int width, int height) {
    std::cout << "\033[2J"; // Clear screen

    for (const auto& p : particles) {
        int x = static_cast<int>(p.position[0]);
        int y = static_cast<int>(p.position[1]);

        // Clamp to terminal bounds
        if (x < 0 || x >= width || y < 0 || y >= height) continue;

        // Move cursor: ANSI escape is 1-based (row;col)
        std::cout << "\033[" << (height - y) << ";" << (2*x + 1) << "H" << ".";
    }

    std::cout.flush();
}

int main() {
    VulkanContext ctx = initVulkan();
	
	// Initialize the Particles 
	Param params;
	params.num_particles 	= 100;
	params.bounds[0] 		= 100.0f;
	params.bounds[1] 		= 56.0f;
	params.pad 				= 0.0f;
	std::vector<Particle> particles(params.num_particles);
	srand(time(0));
	for(size_t i=0; i < params.num_particles; i++){
		for(int j=0; j < 2; j++) {
			particles[i].position[j] = (rand()/ (float)RAND_MAX) * params.bounds[j];
			particles[i].velocity[j] = 0*(1-2*(rand()/ (float)RAND_MAX)) * VELMAG;
		}
	}

    VulkanBuffer buf = createBuffer(ctx, sizeof(Particle)*particles.size(), particles.data());
	VulkanBuffer params_buf = createUniformBuffer(ctx, sizeof(Param), &params); 

	// Run an animation loop
	for (int frame=0; frame < FRAMES; frame++){

		std::cout << "\033[2J\033[H"; // Clear screen and move cursor to top-left

		runComputeShader(ctx, "particle-shader.spv", buf, params_buf, params.num_particles);
		readBuffer(ctx, buf, particles.data());

		printParticlesTerminal(particles, 212,56);
			
		// Sleep a little so that i don't burn my computer
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

    destroyBuffer(ctx, params_buf);
    destroyBuffer(ctx, buf);
    cleanupVulkan(ctx);
    return 0;
}

