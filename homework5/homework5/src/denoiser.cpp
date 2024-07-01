#include "denoiser.h"

Denoiser::Denoiser() : m_useTemportal(false) {}

void Denoiser::Reprojection(const FrameInfo &frameInfo) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    Matrix4x4 preWorldToScreen =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 1];
    Matrix4x4 preWorldToCamera =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 2];
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            m_valid(x, y) = false;
            m_misc(x, y) = Float3(0.f);
            // TODO: Reproject
            Float3 position = frameInfo.m_position(x,y);
            int indexCur = frameInfo.m_id(x, y);
            if(indexCur == -1){
                continue;
            }
            // 根据这帧的位置，和获取存储上帧每个物体矩阵的对应物体矩阵，上帧摄像机矩阵，上帧投影到屏幕矩阵带入公式获得上帧该物体对应像素的屏幕坐标
            Matrix4x4 invWorldMatrixCur = Inverse(frameInfo.m_matrix[indexCur]);
            Matrix4x4 worldMatrixPre = m_preFrameInfo.m_matrix[indexCur];
            Matrix4x4 screenMatrixpre = preWorldToScreen * preWorldToCamera * worldMatrixPre * invWorldMatrixCur;
            Float3 screenPositionPre = screenMatrixpre(position, Float3::EType::Point);

            //判断上帧这个物体的这个部分是否是屏幕外
            if(screenPositionPre.x < 0 || screenPositionPre.x >= width || screenPositionPre.y < 0 || screenPositionPre.y >= height){
                continue;
            }
            else{
                int indexPre = m_preFrameInfo.m_id(screenPositionPre.x, screenPositionPre.y);
                // 判断是否上帧和这帧出现了遮挡关系改变，即上帧对应的位置上是别的物体
                if(indexPre == indexCur){
                    m_valid(x, y) = true;
                    m_misc(x, y) = m_accColor(screenPositionPre.x, screenPositionPre.y);
                }
            }
        }
    }
    std::swap(m_misc, m_accColor);
}

void Denoiser::TemporalAccumulation(const Buffer2D<Float3> &curFilteredColor) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    int kernelRadius = 7;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Temporal clamp
            Float3 color = m_accColor(x, y);
            // TODO: Exponential moving average
            float alpha = 1.0f;
            if(m_valid(x, y)){
                alpha = m_alpha;
                int count = 0;
                Float3 mu(0.f);
                Float3 sigma(0.f);
                int x_start = std::max(0, x - kernelRadius);
                int x_end = std::min(width - 1, x + kernelRadius);
                int y_start = std::max(0, y - kernelRadius);
                int y_end = std::min(height - 1, y + kernelRadius);
                for(int i = x_start; i < x_end; i++){
                    for(int j = y_start; j < y_end; j++){
                        mu += curFilteredColor(i, j);
                        count++;
                    }
                }
                mu /= float(count);
                for(int i = x_start; i < x_end; i++){
                    for(int j = y_start; j < y_end; j++){
                        sigma += Sqr(curFilteredColor(i, j) - mu);
                    }
                }
                sigma = SafeSqrt(sigma / float(count));
                color = Clamp(color, mu - sigma * m_colorBoxK, mu + sigma * m_colorBoxK);
            }
            m_misc(x, y) = Lerp(color, curFilteredColor(x, y), alpha);
        }
    }
    std::swap(m_misc, m_accColor);
}

Buffer2D<Float3> Denoiser::Filter(const FrameInfo &frameInfo) {
    int height = frameInfo.m_beauty.m_height;
    int width = frameInfo.m_beauty.m_width;
    Buffer2D<Float3> filteredImage = CreateBuffer2D<Float3>(width, height);
    int kernelRadius = 16;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Float3 result;
            auto totalWeight = .0f;

            int x_start = std::max(0, x - kernelRadius);
            int x_end = std::min(width - 1, x + kernelRadius);
            int y_start = std::max(0, y - kernelRadius);
            int y_end = std::min(height - 1, y + kernelRadius);

            Float3 centerPosition = frameInfo.m_position(x, y);
            Float3 centerNormal   = frameInfo.m_normal(x, y);
            Float3 centerColor    = frameInfo.m_beauty(x, y);

            for(int i = x_start; i <= x_end; i++) {
                for(int j = y_start; j <= y_end; j++) {
                    Float3 curPosition = frameInfo.m_position(i, j);
                    Float3 curNormal   = frameInfo.m_normal(i, j);
                    Float3 curColor    = frameInfo.m_beauty(i, j);
                    
                    // Position
                    auto positionD = SqrDistance(centerPosition, curPosition) / (2.0f * m_sigmaCoord * m_sigmaCoord);

                    // Color
                    auto colorD = SqrDistance(centerColor, curColor) / (2.0f * m_sigmaColor * m_sigmaColor);

                    // Normal
                    auto normalD = SafeAcos(Dot(centerNormal, curNormal));
                    normalD *= normalD;
                    normalD / (2.0f * m_sigmaNormal * m_sigmaNormal);

                    // Plane
                    float planeD = .0f;
                    if(positionD > 0.f){
                        planeD = Dot(centerNormal, Normalize(curPosition - centerPosition));
                    }
                    planeD *= planeD;
                    planeD /= (2.0f * m_sigmaPlane * m_sigmaPlane);

                    float weight = std::exp(-(positionD + colorD + normalD + planeD));
                    totalWeight += weight;
                    result += curColor * weight;
                }
            }
            // TODO: Joint bilateral filter
            filteredImage(x, y) = result / totalWeight;
        }
    }
    return filteredImage;
}

void Denoiser::Init(const FrameInfo &frameInfo, const Buffer2D<Float3> &filteredColor) {
    m_accColor.Copy(filteredColor);
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    m_misc = CreateBuffer2D<Float3>(width, height);
    m_valid = CreateBuffer2D<bool>(width, height);
}

void Denoiser::Maintain(const FrameInfo &frameInfo) { m_preFrameInfo = frameInfo; }

Buffer2D<Float3> Denoiser::ProcessFrame(const FrameInfo &frameInfo) {
    // Filter current frame
    Buffer2D<Float3> filteredColor;
    filteredColor = Filter(frameInfo);

    // Reproject previous frame color to current
    if (m_useTemportal) {
        Reprojection(frameInfo);
        TemporalAccumulation(filteredColor);
    } else {
        Init(frameInfo, filteredColor);
    }

    // Maintain
    Maintain(frameInfo);
    if (!m_useTemportal) {
        m_useTemportal = true;
    }
    return m_accColor;
}
