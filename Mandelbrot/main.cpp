/**
 * Program Generator Fraktal (Versi Final - GUI + Benchmark)
 * - Mode GUI Interaktif (default)
 * - Mode Benchmark dengan flag --benchmark (Serial, Paralel, GPU)
 * - Resolusi Dinamis, Menyimpan Gambar, Mengunci Julia
 */
#define ENABLE_SFML_GUI
#define ENABLE_OPENCL

#include <iostream>
#include <vector>
#include <complex>
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <omp.h>

#ifdef ENABLE_OPENCL
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>
#endif

#ifdef ENABLE_SFML_GUI
#include <SFML/Graphics.hpp>
#endif

const int DEFAULT_WIDTH = 1920;
const int DEFAULT_HEIGHT = 1080;
const int MAX_ITERATIONS = 1000;

struct Color { uint8_t r, g, b; };

Color map_iteration_to_color(int iterations) {
    if (iterations == MAX_ITERATIONS) return {0, 0, 0};
    float t = static_cast<float>(iterations) / MAX_ITERATIONS;
    uint8_t r = static_cast<uint8_t>(9 * (1 - t) * t * t * t * 255);
    uint8_t g = static_cast<uint8_t>(15 * (1 - t) * (1 - t) * t * t * 255);
    uint8_t b = static_cast<uint8_t>(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    return {r, g, b};
}

// =======================================================================================
// FUNGSI-FUNGSI GENERATOR FRAKTAL (SERIAL, PARALEL, GPU)
// =======================================================================================

// --- Implementasi Serial (DIKEMBALIKAN) ---
void generate_fractal_serial(
    std::vector<uint8_t>& pixels, int width, int height,
    float min_re, float max_re, float min_im, float max_im)
{
    float re_range = max_re - min_re;
    float im_range = max_im - min_im;

    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            float cx = min_re + static_cast<float>(px) / (width - 1) * re_range;
            float cy = min_im + static_cast<float>(py) / (height - 1) * im_range;

            std::complex<float> z = {0,0};
            std::complex<float> c = {cx, cy};

            int iterations = 0;
            while (iterations < MAX_ITERATIONS) {
                if (std::abs(z) > 2.0f) break;
                z = z * z + c;
                iterations++;
            }

            Color color = map_iteration_to_color(iterations);
            size_t index = (py * width + px) * 3;
            pixels[index]     = color.r;
            pixels[index + 1] = color.g;
            pixels[index + 2] = color.b;
        }
    }
}


// --- Implementasi Paralel CPU ---
void generate_fractal_parallel(
    std::vector<uint8_t>& pixels, int width, int height,
    float min_re, float max_re, float min_im, float max_im,
    bool is_julia = false, std::complex<float> julia_c = {0,0})
{
    float re_range = max_re - min_re;
    float im_range = max_im - min_im;

    #pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            float cx = min_re + static_cast<float>(px) / (width - 1) * re_range;
            float cy = min_im + static_cast<float>(py) / (height - 1) * im_range;

            std::complex<float> z, c;

            if (is_julia) {
                z = {cx, cy}; c = julia_c;
            } else {
                z = {0,0}; c = {cx, cy};
            }

            int iterations = 0;
            while (iterations < MAX_ITERATIONS) {
                if (std::abs(z) > 2.0f) break;
                z = z * z + c;
                iterations++;
            }

            Color color = map_iteration_to_color(iterations);
            size_t index = (py * width + px) * 3;
            pixels[index]     = color.r;
            pixels[index + 1] = color.g;
            pixels[index + 2] = color.b;
        }
    }
}

// --- Implementasi GPU ---
#ifdef ENABLE_OPENCL
void generate_fractal_gpu(
    std::vector<uint8_t>& pixels, int width, int height,
    float min_re, float max_re, float min_im, float max_im,
    bool is_julia, std::complex<float> julia_c)
{
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) throw std::runtime_error("No OpenCL platform found.");

        cl::Device device;
        for(auto& p : platforms) {
            std::vector<cl::Device> p_devices;
            p.getDevices(CL_DEVICE_TYPE_GPU, &p_devices);
            if(!p_devices.empty()) {
                device = p_devices.front();
                break;
            }
        }
        if(!device()) { // Fallback to CPU if no GPU found
            for(auto& p : platforms) {
                std::vector<cl::Device> p_devices;
                p.getDevices(CL_DEVICE_TYPE_CPU, &p_devices);
                if(!p_devices.empty()) {
                    device = p_devices.front();
                    break;
                }
            }
        }
        if(!device()) throw std::runtime_error("No OpenCL device found.");

        std::cout << "[OpenCL] Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        cl::Context context(device);
        cl::CommandQueue queue(context, device);

        std::ifstream kernel_file("mandelbrot_kernel.cl");
        if (!kernel_file.is_open()) throw std::runtime_error("Failed to open kernel file.");
        std::string kernel_code(std::istreambuf_iterator<char>(kernel_file), (std::istreambuf_iterator<char>()));

        cl::Program program(context, kernel_code);
        try {
            program.build({device});
        } catch (const cl::Error& e) {
            if (e.err() == CL_BUILD_PROGRAM_FAILURE) {
                std::cerr << "=== OpenCL Build Log ===\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
            }
            throw;
        }

        cl::Buffer output_buffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * width * height);
        cl::Kernel kernel(program, "generate_fractal");
        kernel.setArg(0, output_buffer); kernel.setArg(1, width); kernel.setArg(2, height);
        kernel.setArg(3, min_re); kernel.setArg(4, max_re); kernel.setArg(5, min_im); kernel.setArg(6, max_im);
        kernel.setArg(7, MAX_ITERATIONS); kernel.setArg(8, static_cast<int>(is_julia));
        kernel.setArg(9, julia_c.real()); kernel.setArg(10, julia_c.imag());

        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width * height), cl::NullRange);

        std::vector<int> iteration_results(width * height);
        queue.enqueueReadBuffer(output_buffer, CL_TRUE, 0, sizeof(int) * width * height, iteration_results.data());
        queue.finish();

        #pragma omp parallel for
        for (int i = 0; i < width * height; ++i) {
            Color color = map_iteration_to_color(iteration_results[i]);
            pixels[i * 3] = color.r; pixels[i * 3 + 1] = color.g; pixels[i * 3 + 2] = color.b;
        }
    } catch (const cl::Error& e) {
        std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << '\n';
    }
}
#endif

// =======================================================================================
// MODE OPERASI PROGRAM
// =======================================================================================

// --- FUNGSI BARU: Mode Benchmark ---
void run_benchmarks(int width, int height) {
    std::cout << "=================================================\n";
    std::cout << "            MODE BENCHMARK AKTIF\n";
    std::cout << "=================================================\n";
    std::cout << "Resolusi: " << width << "x" << height << ", Iterasi Maks: " << MAX_ITERATIONS << "\n\n";

    std::vector<uint8_t> pixels(width * height * 3);
    float min_re = -2.0f, max_re = 1.0f, min_im = -1.2f;
    float max_im = min_im + (max_re - min_re) * static_cast<float>(height) / width;

    // 1. Benchmark Serial
    std::cout << "[1] Menjalankan benchmark Serial..." << std::flush;
    auto start_serial = std::chrono::high_resolution_clock::now();
    generate_fractal_serial(pixels, width, height, min_re, max_re, min_im, max_im);
    auto end_serial = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> serial_time = end_serial - start_serial;
    stbi_write_png("fractal_serial.png", width, height, 3, pixels.data(), width * 3);
    std::cout << " Selesai.\n";

    // 2. Benchmark Paralel (OpenMP)
    std::cout << "[2] Menjalankan benchmark Paralel (OpenMP)..." << std::flush;
    auto start_parallel = std::chrono::high_resolution_clock::now();
    generate_fractal_parallel(pixels, width, height, min_re, max_re, min_im, max_im);
    auto end_parallel = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> parallel_time = end_parallel - start_parallel;
    stbi_write_png("fractal_parallel_omp.png", width, height, 3, pixels.data(), width * 3);
    std::cout << " Selesai.\n";

    // 3. Benchmark GPU (OpenCL)
    #ifdef ENABLE_OPENCL
    std::cout << "[3] Menjalankan benchmark GPU (OpenCL)..." << std::flush;
    auto start_gpu = std::chrono::high_resolution_clock::now();
    generate_fractal_gpu(pixels, width, height, min_re, max_re, min_im, max_im, false, {0,0});
    auto end_gpu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> gpu_time = end_gpu - start_gpu;
    stbi_write_png("fractal_gpu_opencl.png", width, height, 3, pixels.data(), width * 3);
    std::cout << " Selesai.\n";
    #endif

    // 4. Tampilkan Hasil
    std::cout << "\n================  HASIL BENCHMARK  ================\n";
    std::cout << "Waktu Eksekusi Serial           : " << std::fixed << std::setprecision(2) << serial_time.count() << " ms\n";
    std::cout << "Waktu Eksekusi Paralel (OpenMP) : " << parallel_time.count() << " ms\n";
    #ifdef ENABLE_OPENCL
    std::cout << "Waktu Eksekusi GPU (OpenCL)     : " << gpu_time.count() << " ms\n";
    #endif
    std::cout << "-------------------------------------------------\n";
    std::cout << "Rasio Percepatan (OpenMP vs Serial) : " << (serial_time.count() / parallel_time.count()) << "x\n";
    #ifdef ENABLE_OPENCL
    std::cout << "Rasio Percepatan (OpenCL vs Serial) : " << (serial_time.count() / gpu_time.count()) << "x\n";
    #endif
    std::cout << "=================================================\n";
    std::cout << "Gambar output: fractal_serial.png, fractal_parallel_omp.png, fractal_gpu_opencl.png\n";
}

// --- Mode GUI Interaktif ---
#ifdef ENABLE_SFML_GUI
void run_interactive_gui(int width, int height) {
    sf::RenderWindow window(sf::VideoMode(width, height), "Interactive Fractal Explorer | Gemini");
    window.setFramerateLimit(60);

    sf::Image image; image.create(width, height, sf::Color::Black);
    sf::Texture texture; sf::Sprite sprite;

    std::vector<uint8_t> pixels(width * height * 4);

    float min_re = -2.0f, max_re = 1.0f;
    float min_im = -1.2f, max_im = min_im + (max_re - min_re) * static_cast<float>(height) / width;
    bool needs_redraw = true;
    bool is_julia = false;
    std::complex<float> julia_c(-0.7f, 0.27015f);
    bool julia_locked = false;

    bool is_zooming = false;
    sf::Vector2f zoom_start_pos;
    sf::RectangleShape zoom_rect;
    zoom_rect.setFillColor(sf::Color(100, 100, 255, 50));
    zoom_rect.setOutlineColor(sf::Color::White);
    zoom_rect.setOutlineThickness(1.f);

    bool rightDragging = false;
    sf::Vector2i lastMousePos;

    std::cout << "\nEntering Interactive Mode (" << width << "x" << height << ")...\n"
              << "---------------------------\n"
              << "Controls:\n"
              << "  - Left Click + Drag : Zoom to area\n"
              << "  - Right Click + Drag: Pan image\n"
              << "  - 'J' Key           : Toggle Mandelbrot/Julia set\n"
              << "  - 'L' Key           : Lock/Unlock Julia set constant 'c'\n"
              << "  - 'S' Key           : Save current view to PNG file\n"
              << "  - 'R' Key           : Reset view\n"
              << "  - Mouse Move        : (Julia Mode) Change 'c' constant\n"
              << "---------------------------\n\n";

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::J) {
                    is_julia = !is_julia; needs_redraw = true;
                    std::cout << "Mode switched to: " << (is_julia ? "Julia" : "Mandelbrot") << std::endl;
                }
                if (event.key.code == sf::Keyboard::R) {
                    min_re = -2.0f; max_re = 1.0f;
                    min_im = -1.2f; max_im = min_im + (max_re - min_re) * static_cast<float>(height) / width;
                    needs_redraw = true;
                }
                if (event.key.code == sf::Keyboard::S) {
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << "fractal_" << std::put_time(std::localtime(&now_time), "%Y-%m-%d_%H-%M-%S") << ".png";
                    if (image.saveToFile(ss.str())) std::cout << "Image saved to " << ss.str() << std::endl;
                    else std::cerr << "Error: Failed to save image to " << ss.str() << std::endl;
                }
                if (event.key.code == sf::Keyboard::L) {
                    if (is_julia) {
                        julia_locked = !julia_locked;
                        std::cout << "Julia set constant 'c' is now " << (julia_locked ? "LOCKED" : "UNLOCKED") << std::endl;
                        std::string title = "Interactive Fractal Explorer | Julia Set";
                        if(julia_locked) title += " (Locked)";
                        window.setTitle(title);
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    is_zooming = true;
                    zoom_start_pos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    rightDragging = true;
                    lastMousePos = {event.mouseButton.x, event.mouseButton.y};
                }
            }
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left && is_zooming) {
                    is_zooming = false;
                    sf::Vector2f zoom_end_pos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                    if (zoom_start_pos.x != zoom_end_pos.x && zoom_start_pos.y != zoom_end_pos.y) {
                        float re_range = max_re - min_re;
                        float new_min_re = min_re + (std::min(zoom_start_pos.x, zoom_end_pos.x) / width) * re_range;
                        float new_max_re = min_re + (std::max(zoom_start_pos.x, zoom_end_pos.x) / width) * re_range;
                        float new_min_im = min_im + (std::min(zoom_start_pos.y, zoom_end_pos.y) / height) * (max_im - min_im);
                        max_im = new_min_im + (new_max_re - new_min_re) * static_cast<float>(height) / width;
                        min_re = new_min_re; max_re = new_max_re; min_im = new_min_im;
                        needs_redraw = true;
                    }
                } else if (event.mouseButton.button == sf::Mouse::Right) rightDragging = false;
            }
            // ===== BLOK BARU YANG SUDAH DIPERBAIKI =====
            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2i currentMousePos(event.mouseMove.x, event.mouseMove.y);

                // --- Logika untuk menggambar persegi panjang zoom ---
                if (is_zooming) {
                    // Konversi posisi mouse saat ini ke koordinat dunia/view
                    sf::Vector2f currentWorldPos = window.mapPixelToCoords(currentMousePos);

                    // Tentukan pojok kiri-atas dan ukuran persegi secara dinamis
                    // agar berfungsi ke segala arah drag
                    float left = std::min(zoom_start_pos.x, currentWorldPos.x);
                    float top = std::min(zoom_start_pos.y, currentWorldPos.y);
                    float width = std::abs(zoom_start_pos.x - currentWorldPos.x);
                    float height = std::abs(zoom_start_pos.y - currentWorldPos.y);

                    // Atur posisi dan ukuran persegi panjang zoom
                    zoom_rect.setPosition(left, top);
                    zoom_rect.setSize({width, height});
                }

                // --- Logika untuk Pan/Geser (klik kanan) ---
                if (rightDragging) {
                    sf::Vector2i delta = currentMousePos - lastMousePos;
                    float dx = -static_cast<float>(delta.x) * (max_re - min_re) / width;
                    float dy = -static_cast<float>(delta.y) * (max_im - min_im) / height;
                    min_re += dx; max_re += dx; min_im += dy; max_im += dy;
                    needs_redraw = true;
                    lastMousePos = currentMousePos;
                }

                // --- Logika untuk update konstanta Julia ---
                if (is_julia && !julia_locked) {
                    julia_c.real(min_re + (static_cast<float>(currentMousePos.x) / width) * (max_re - min_re));
                    julia_c.imag(min_im + (static_cast<float>(currentMousePos.y) / height) * (max_im - min_im));
                    needs_redraw = true;
                }
            }
        }

        if (needs_redraw) {
            std::cout << "Rendering... " << std::flush;
            auto start_render = std::chrono::high_resolution_clock::now();
            std::vector<uint8_t> temp_pixels(width * height * 3);
            #ifdef ENABLE_OPENCL
            generate_fractal_gpu(temp_pixels, width, height, min_re, max_re, min_im, max_im, is_julia, julia_c);
            #else
            generate_fractal_parallel(temp_pixels, width, height, min_re, max_re, min_im, max_im, is_julia, julia_c);
            #endif
            for(int i = 0; i < width * height; ++i) {
                pixels[i*4 + 0] = temp_pixels[i*3 + 0]; pixels[i*4 + 1] = temp_pixels[i*3 + 1];
                pixels[i*4 + 2] = temp_pixels[i*3 + 2]; pixels[i*4 + 3] = 255;
            }
            image.create(width, height, pixels.data());
            texture.loadFromImage(image);
            sprite.setTexture(texture);
            auto end_render = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> render_time = end_render - start_render;
            std::cout << "Done in " << render_time.count() << " ms." << std::endl;
            needs_redraw = false;
        }

        window.clear();
        window.draw(sprite);
        if(is_zooming) window.draw(zoom_rect);
        window.display();
    }
}
#endif

// --- Fungsi Main dengan Pemilihan Mode ---
int main(int argc, char* argv[]) {
    bool benchmark_mode = false;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;

    // Parsing argumen command-line
    if (argc > 1) {
        std::string first_arg = argv[1];
        if (first_arg == "--benchmark") {
            benchmark_mode = true;
            if (argc == 4) { // ./prog --benchmark 1920 1080
                try {
                    width = std::stoi(argv[2]);
                    height = std::stoi(argv[3]);
                } catch(...) { /* biarkan default jika parsing gagal */ }
            }
        } else { // ./prog 1920 1080
            if (argc == 3) {
                try {
                    width = std::stoi(argv[1]);
                    height = std::stoi(argv[2]);
                } catch(...) { /* biarkan default jika parsing gagal */ }
            }
        }
    }

    if (benchmark_mode) {
        run_benchmarks(width, height);
    } else {
        #ifdef ENABLE_SFML_GUI
            run_interactive_gui(width, height);
        #else
            std::cout << "Mode GUI dinonaktifkan. Menjalankan benchmark sebagai gantinya...\n";
            run_benchmarks(width, height);
        #endif
    }

    return 0;
}
