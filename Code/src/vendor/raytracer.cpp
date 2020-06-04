#include <raytracer.hpp>
#include <thread>
#include <mutex>
#include <fstream>

raytracer::raytracer(int width, int height, glm::mat4 const& proj_matrix, callback_t intersect_triangle) : width(width), height(height), proj_matrix(proj_matrix), callback(intersect_triangle) {
    // compute near / far
    float m22 = -proj_matrix[2][2];
    float m32 = -proj_matrix[3][2];

    far_value = (2.0f*m32)/(2.0f*m22-2.0f);
    near_value = ((m22-1.0f)*far_value)/(m22+1.0);
}

void
raytracer::trace(glm::mat4 const& view_matrix, const char* filename) {
    // compute camera position
    glm::mat3 R(view_matrix);
    glm::vec3 t(view_matrix[3]);
    glm::vec3 cam_pos = -glm::transpose(R) * t;

    int32_t num_threads = std::thread::hardware_concurrency() - 1;
    int32_t chunk_size = height / num_threads;
    if (height % num_threads) ++chunk_size;

    int32_t thread_idx = 0;
    uint32_t col_count = 0;
    std::vector<std::thread> threads(num_threads);
    std::mutex print_mutex;
    std::vector<uint8_t> img_data(width*height*3);
    for (int first = 0; first < height; first += chunk_size) {
        int last = std::min(first + chunk_size, height);

        threads[thread_idx++] = std::thread([&, first, last]() {
            for (int row = first; row < last; ++row) {
                for (int col = 0; col < width; ++col) {
                    glm::vec3 win(static_cast<float>(col), static_cast<float>(row), near_value);
                    glm::vec3 near_pos = glm::unProject(win,
                                                        view_matrix,
                                                        proj_matrix,
                                                        glm::uvec4(0, 0, width, height));
                    glm::vec3 dir = glm::normalize(near_pos - cam_pos);


                    intersection isect;
                    isect.lambda = far_value;
                    int j = height - row - 1;
                    if (callback(ray{cam_pos, dir}, &isect)) {
                        img_data[j * width * 3 + col * 3 + 0] = static_cast<uint8_t>(255.f * isect.color.x);
                        img_data[j * width * 3 + col * 3 + 1] = static_cast<uint8_t>(255.f * isect.color.y);
                        img_data[j * width * 3 + col * 3 + 2] = static_cast<uint8_t>(255.f * isect.color.z);
                    } else {
                        // background color
                        img_data[j * width * 3 + col * 3 + 0] = static_cast<uint8_t>(255.f * 0.25f);
                        img_data[j * width * 3 + col * 3 + 1] = static_cast<uint8_t>(255.f * 0.25f);
                        img_data[j * width * 3 + col * 3 + 2] = static_cast<uint8_t>(255.f * 0.25f);
                    }
                }
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cout << (++col_count) << "/" << height << "\n";
            }
        });
    }
    // wait for all threads
    for (thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        threads[thread_idx].join();
    }

    // write PPM
    std::ofstream out(filename);
    // header
    out << "P6 " << width << " " << height << " 255\n";
    out.close();
    // data
    out.open(filename, std::ios::app | std::ios::binary | std::ios::out);
    out.write((const char*)img_data.data(), width*height*3);

    //img.write(filename);

}
