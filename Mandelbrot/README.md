# Fractal Explorer C++

Program C++ untuk menghasilkan fraktal Himpunan Mandelbrot dan Julia dan melihatnya secara realtime.

<img width="1920" height="1080" alt="fractal_2025-08-15_21-38-28" src="https://github.com/user-attachments/assets/7157da74-6e0f-4db2-b413-27cbc07018df" />

---
## Fitur Utama

-   **Multiple Rendering:** Tiga rendering: Serial (single-thread), Paralel CPU (OpenMP), dan GPU (OpenCL).
-   **GUI:** Dengan SFML, tampilan fraktal *real-time*.
-   **Navigasi:** Bisa **Zoom** (klik kiri + tarik) dan **Pan/Geser** (klik kanan + tarik).
-   **Himpunan Mandelbrot & Julia:** Berganti antara kedua himpunan fraktal dengan menekan tombol **'J'**.
-   **Dynamic Julia Set:** Konstanta `c` untuk himpunan Julia dapat diubah secara *real-time* dengan menggerakkan mouse.
-   **Kunci:** Meng-freeze (*lock*) konstanta `c` pada himpunan Julia dengan tombol **'L'**.
-   **Resolusi Dinamis:** Tentukan resolusi rendering melalui argumen *command-line* `./fractal_generator 1920 1080`.
-   **Simpan ke File:** Simpan tampilan fraktal saat ini ke file `.png` dengan nama berdasarkan *timestamp* melalui tombol **'S'**.
-   **Mode Benchmark:** Mode tambahaan untuk membandingkan performa antara implementasi Serial, OpenMP, dan OpenCL `./fractal_generator --benchmark`.

---
## Requirement

1.  **Compiler C++:** `g++` dengan dukungan C++17 dan OpenMP.
2.  **Library SFML:** Versi *development* dari SFML 2.5 atau lebih baru.
    -   Di Ubuntu/Debian: `sudo apt install libsfml-dev`
3.  **Driver & Header OpenCL:** Driver untuk GPU (NVIDIA, AMD, atau Intel) dan header OpenCL.
    -   Di Ubuntu/Debian: `sudo apt install opencl-headers ocl-icd-opencl-dev`

---
## Penjelasan Implementasi

#### Bagian 1: Implementasi Serial
Implementasi dasar untuk perbandingan performa. Setiap piksel dihitung satu per satu dalam satu *thread* CPU. Logika utamanya adalah iterasi rumus $Z_{n+1} = Z_n^2 + c$.

#### Bagian 2: Paralelisasi CPU (OpenMP)
Menggunakan OpenMP (`#pragma omp parallel for schedule(dynamic)`) untuk memparalelkan *loop* terluar pada perhitungan piksel. Ini mempercepat rendering dengan mendistribusikan beban kerja ke semua core CPU yang tersedia.

#### Bagian 3: Akselerasi GPU (OpenCL)
Memanfaatkan GPU. *Kernel* OpenCL (`.cl`) di-compile saat runtime dan dieksekusi di GPU. Setiap *work-item* GPU bertanggung jawab untuk menghitung satu piksel, sehingga bisa ratusan atau ribuan piksel dihitung secara bersamaan.

#### Bagian 4 & 5: GUI Interaktif (SFML) & Himpunan Julia
GUI dengan SFML. Mode Himpunan Julia memungkinkan posisi kursor mouse secara real-time mengontrol bentuk fraktal.

---
## Compile Program

### Compiling
Buka terminal di subfolder ini dan jalankan command:
```bash
g++ main.cpp -o fractal_generator -std=c++17 -O3 -Wall -fopenmp -lsfml-graphics -lsfml-window -lsfml-system -lOpenCL
```

### Menjalankan Program

#### Mode 1: GUI (Default)
Jalankan program tanpa flag atau hanya dengan argumen resolusi.

-   **Resolusi Default (1920x1080):**
    ```bash
    ./fractal_generator
    ```
-   **Resolusi Custom (misal: 1280x720):**
    ```bash
    ./fractal_generator 1280 720
    ```

#### Mode 2: Benchmark
Pakai *flag* `--benchmark` untuk menjalankan tes performa.

-   **Benchmark dengan Resolusi Default:**
    ```bash
    ./fractal_generator --benchmark
    ```
-   **Benchmark dengan Resolusi Custom:**
    ```bash
    ./fractal_generator --benchmark 1920 1080
    ```
Mode ini akan mengoutput hasil ke terminal dan menyimpan tiga gambar (`fractal_serial.png`, `fractal_parallel_omp.png`, `fractal_gpu_opencl.png`).

---
## Hasil Benchmark

<img width="764" height="372" alt="image" src="https://github.com/user-attachments/assets/1cdca733-8e2f-4aaa-87bd-f8931493d267" />

---
## Galeri

### Screenshot

| Himpunan Mandelbrot (Tampilan Awal) | Himpunan Julia |
| :---------------------------------: | :---------------------: |
|     <img width="1920" height="1080" alt="fractal_2025-08-15_21-38-28" src="https://github.com/user-attachments/assets/69e450fd-04ef-4195-a70a-a99985705ab7" />   |   <img width="1920" height="1080" alt="fractal_2025-08-15_21-38-16" src="https://github.com/user-attachments/assets/22a11271-12a0-4faf-bd93-dcb3e84695ff" />   |

### Video Demonstrasi

https://youtu.be/uVI_haYTjoo
