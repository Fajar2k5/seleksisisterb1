/*
 * mandelbrot_kernel.cl (Final float version)
 * Kernel OpenCL untuk menghitung iterasi himpunan Mandelbrot atau Julia.
 */

__kernel void generate_fractal(
    __global int* output,
    const int width,
    const int height,
    const float min_re,
    const float max_re,
    const float min_im,
    const float max_im,
    const int max_iterations,
    const int is_julia,
    const float julia_c_re,
    const float julia_c_im
) {
    int gid = get_global_id(0);
    if (gid >= width * height) return;

    int px = gid % width;
    int py = gid / width;

    float re_range = max_re - min_re;
    float im_range = max_im - min_im;
    float cx = min_re + (float)px / (width - 1) * re_range;
    float cy = min_im + (float)py / (height - 1) * im_range;

    float z_re, z_im;
    float c_re, c_im;

    if (is_julia) {
        z_re = cx;
        z_im = cy;
        c_re = julia_c_re;
        c_im = julia_c_im;
    } else {
        z_re = 0.0f;
        z_im = 0.0f;
        c_re = cx;
        c_im = cy;
    }

    int iterations = 0;
    while (iterations < max_iterations) {
        if ((z_re * z_re + z_im * z_im) > 4.0f) { // Gunakan 4.0f
            break;
        }
        float temp_z_re = z_re * z_re - z_im * z_im + c_re;
        z_im = 2.0f * z_re * z_im + c_im; // Gunakan 2.0f
        z_re = temp_z_re;
        iterations++;
    }
    output[gid] = iterations;
}
