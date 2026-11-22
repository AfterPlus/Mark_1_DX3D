#pragma once

// Drift scoring, in the spirit of the Tokyo Drift reference game: the score comes from the TILT of the car relative to its
// direction of travel (the slip angle), scaled by speed, and chained into a combo.
//
//   * A drift counts while the slide angle is between MIN and MAX (a bigger angle than that is a spin: no points),
//     the car is fast enough and still on the road.
//   * Points per second = angle (degrees) * speed (m/s) * POINTS_RATE * multiplier.
//   * The multiplier grows the longer the chain lasts without a break.
//   * When the drift ends there is a short grace period (so a quick transition from one drift to the next keeps the chain);
//     after that the chain is BANKED into the total score.
//   * Hitting a wall or leaving the road loses the chain that was running.
//
// Pure game logic: no graphics, no physics.
class DriftScore
{
public:
    DriftScore();

    void Reset();                                                   // new run: clears everything
    void Update(float dt, float slipAngle, float speed, bool onRoad);
    void Crash();                                                   // loses the chain that is running (the banked total stays)

    int GetTotal() const { return m_total; }                        // banked points
    int GetChain() const { return (int)m_chain; }                   // points in the running chain, not banked yet
    float GetMultiplier() const { return m_multiplier; }
    bool IsDrifting() const { return m_drifting; }
    bool IsChainActive() const { return m_active; }
    float GetAngleDegrees() const { return m_angleDegrees; }
    int GetBestChain() const { return m_bestChain; }

private:
    void Bank();

private:
    float m_chain;
    float m_chainTime;
    float m_multiplier;
    float m_graceTimer;
    float m_angleDegrees;
    bool m_drifting;
    bool m_active;
    int m_total;
    int m_bestChain;
};
