// === main.cpp ===
#include "vulkan_context.h"
#include "compute_pipeline.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>


// Some cool global constants
const int FRAMES 		= 100000;
const int NUM 			= 100;
const float BOUNDS[2] 	= {200,50};
const float VELMAG 		= 5;

// Create the struct for the particles
struct Particle {
	float position[2];
	float velocity[2];
};


void printParticlesTerminal(const std::vector<Particle>& particles, int width, int height) {
    std::cout << "\033[2J"; // Clear screen

    for (const auto& p : particles) {
        int x = static_cast<int>(p.position[0]);
        int y = static_cast<int>(p.position[1]);

        // Clamp to terminal bounds
        if (x < 0 || x >= width || y < 0 || y >= height) continue;

        // Move cursor: ANSI escape is 1-based (row;col)
        std::cout << "\033[" << (height - y) << ";" << (x + 1) << "H" << "*";
    }

    std::cout.flush();
}

int main() {
    VulkanContext ctx = initVulkan();
	
	// Initialize the Particles 
	std::vector<Particle> particles(NUM);
	srand(time(0));
	for(size_t i=0; i < NUM; i++){
		for(int j=0; j < 2; j++) {
			particles[i].position[j] = (rand()/ (float)RAND_MAX) * BOUNDS[j];
			particles[i].velocity[j] = (1-2*(rand()/ (float)RAND_MAX)) * VELMAG;
		}
	}

    VulkanBuffer buf = createBuffer(ctx, sizeof(Particle)*particles.size(), particles.data());
	

	// Run an animation loop
	for (int frame=0; frame < FRAMES; frame++){

		std::cout << "\033[2J\033[H"; // Clear screen and move cursor to top-left

		runComputeShader(ctx, "particle-shader.spv", buf, NUM);
		readBuffer(ctx, buf, particles.data());

		printParticlesTerminal(particles, BOUNDS[0], BOUNDS[1]);
		//std::cout << "Frame " << frame << std::endl;
		//for (size_t i = 0; i < 50; i++) {
		//	std::cout << i << " x: " << particles[i].position[0]
		//			  << " y: " << particles[i].position[1] << "\n";
		//}

		// Sleep a little so that i don't burn my computer
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

    
    destroyBuffer(ctx, buf);
    cleanupVulkan(ctx);
    return 0;
}

