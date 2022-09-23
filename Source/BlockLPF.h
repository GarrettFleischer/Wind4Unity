#pragma once

namespace Wind4Unity
{
    class BlockLPF
    {
    public:
        BlockLPF();
        ~BlockLPF();

        void prepare(float cutoff, int samplesPerBlock, double sampleRate);

        float processSample(float input);

    private:
        float coeff{0.0f};
        float lastOutput{0.0f};
    };
}
