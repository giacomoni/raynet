#ifdef ORCA
#include "Orca.h"

using namespace inet::tcp;
using namespace inet;

Register_Class(Orca);

simsignal_t Orca::avg_thrSignal = cComponent::registerSignal("avg_thr");
simsignal_t Orca::thr_cntSignal = cComponent::registerSignal("thr_cnt");
simsignal_t Orca::max_bwSignal = cComponent::registerSignal("max_bw");
simsignal_t Orca::pacing_rateSignal = cComponent::registerSignal("pacing_rate");
simsignal_t Orca::lost_bytesSignal = cComponent::registerSignal("lost_bytes");
simsignal_t Orca::orca_cntSignal = cComponent::registerSignal("orca_cnt");
simsignal_t Orca::min_rttSignal = cComponent::registerSignal("min_rtt");
simsignal_t Orca::avg_urttSignal = cComponent::registerSignal("avg_urtt");

simsignal_t Orca::feature1Signal = cComponent::registerSignal("feature1");
simsignal_t Orca::feature2Signal = cComponent::registerSignal("feature2");
simsignal_t Orca::feature3Signal = cComponent::registerSignal("feature3");
simsignal_t Orca::feature4Signal = cComponent::registerSignal("feature4");
simsignal_t Orca::feature5Signal = cComponent::registerSignal("feature5");
simsignal_t Orca::feature6Signal = cComponent::registerSignal("feature6");
simsignal_t Orca::feature7Signal = cComponent::registerSignal("feature7");
simsignal_t Orca::rewardSignal = cComponent::registerSignal("reward");
simsignal_t Orca::actionSignal = cComponent::registerSignal("action");

Orca::Orca() : TcpCubic(), RLInterface(), state(
                                              (OrcaStateVariables *&)TcpAlgorithm::state)
{
}

std::string OrcaStateVariables::str() const
{
    std::stringstream out;
    out << TcpCubicStateVariables::str();
    return out.str();
}

std::string OrcaStateVariables::detailedInfo() const
{
    std::stringstream out;
    out << TcpCubicStateVariables::detailedInfo();
    return out.str();
}

void Orca::resetStepVariables()
{
    state->orca_cnt = 0;
    state->avg_urtt = 0;
    state->thr_cnt = 0;
    state->avg_thr = 0;
    state->pre_lost_bytes = state->lost_bytes;
}

void Orca::initialize()
{
    //cout << "Initialise TCP newReno" << endl;
    TcpCubic::initialize();
    //TODO: initialization of variables
}

void Orca::established(bool active)
{
    TcpCubic::established(active);
    //TODO: Move this to established()
    if (!rlInitialised)
    {
        initRLAgent();
        // Send the initial step size along
        cObject *simtime = new cSimTime(0.02);
        state->last_mi_t = simTime();
        conn->emit(this->registerSig, stringId.c_str(), simtime);
    }
}

void Orca::receivedDataAck(uint32_t firstSeqAcked)
{
    TcpCubic::receivedDataAck(firstSeqAcked);

    uint64_t interval_us;
    //Update interarrival rate of ACKs

    if (state->last_ack_arrival > 0)
    {
        interval_us = (simTime() - state->last_ack_arrival).inUnit(SIMTIME_US);
    }
    else
    {
        interval_us = 0;
    }

    state->last_ack_arrival = simTime();

    //Update Number of lost packets
    if (state->sackedBytes_old < state->sackedBytes)
    {
        state->lost_bytes += state->snd_mss;
    }

    //Update avg throughput
    if (interval_us > 0)
    {

        uint64_t bw = 1 * THR_UNIT_DEEPCC;
        bw /= interval_us;
        state->avg_thr_raw = state->avg_thr_raw * state->thr_cnt + bw;
        state->thr_cnt++;
        state->avg_thr_raw /= state->thr_cnt;

        state->avg_thr = state->avg_thr_raw * state->snd_mss * 1000000 >> THR_SCALE_DEEPCC;
        conn->emit(avg_thrSignal, state->avg_thr);
    }

    if (state->avg_thr > state->max_bw)
        state->max_bw = state->avg_thr;

    conn->emit(max_bwSignal, state->max_bw);

    //Update pacing rate
    uint64_t rate;
    rate = 1500; //
    rate *= 1000000;
    rate *= max(state->snd_cwnd, state->pipe);
    rate /= (state->srtt.inUnit(SIMTIME_US) >> 3);

    state->pacing_rate = std::min(rate, state->max_pacing_rate);

    conn->emit(pacing_rateSignal, state->pacing_rate);

    // Update sending pace
    auto pacedConn = check_and_cast<PacedTcpConnection *>(this->conn);

    // pacedConn->changeIntersendingTime(1.0 / (state->pacing_rate / state->snd_mss));
    pacedConn->changeIntersendingTime(0);

    // Update RTT
    if (state->last_rtt.inUnit(SIMTIME_US) > 0)
    {

        if (state->min_rtt == 0 || state->min_rtt > state->last_rtt.inUnit(SIMTIME_US))
            state->min_rtt = state->last_rtt.inUnit(SIMTIME_US);

        conn->emit(min_rttSignal, state->min_rtt);

        if (state->last_rtt.inUnit(SIMTIME_US) > 0)
        {
            uint64_t tmp_avg = 0;
            uint64_t tmp2_avg = 0;
            tmp_avg = (state->orca_cnt) * state->avg_urtt + state->last_rtt.inUnit(SIMTIME_US);
            state->orca_cnt++;
            tmp2_avg = state->orca_cnt;
            tmp2_avg = tmp_avg / state->orca_cnt;
            state->avg_urtt = (uint32_t)(tmp2_avg);
            conn->emit(avg_urttSignal, state->avg_urtt);
            conn->emit(orca_cntSignal, state->orca_cnt);
        }
    }

    if (state->snd_una == state->snd_max)
        done = true;
}

void Orca::receivedDuplicateAck()
{
    TcpCubic::receivedDuplicateAck();

    if (state->dupacks == state->dupthresh)
    {
        state->lost_bytes += state->snd_mss;
        conn->emit(lost_bytesSignal, state->lost_bytes);
    }
    else if (state->dupacks > state->dupthresh)
    {
        // We have received new sack information, so one packet was lost on the way
        if (state->sackedBytes_old < state->sackedBytes)
        {
            state->lost_bytes += state->snd_mss;
            conn->emit(lost_bytesSignal, state->lost_bytes);
        }
    }
}

void Orca::initRLAgent()
{
    this->setOwner(this->conn);
    RLInterface::initialise();

    // Generate ID for this agent
    std::string s(this->conn->getFullPath());
    std::string token = s.substr(s.find("-") + 1);

    this->setStringId(token);
}

ObsType Orca::computeObservation()
{   
    // Step is valid if not in slow start and ack have been received in the MI and the connection
    // is not in loss recovery state. 
    if (state->orca_cnt > 0 && !state->lossRecovery)
    {

        if(state->snd_cwnd >= state->ssthresh){
        double feature1, feature2, feature3, feature4, feature5, feature6, feature7;

        double loss_rate = (double)(state->lost_bytes - state->pre_lost_bytes) / ((simTime().inUnit(SIMTIME_US) - state->last_mi_t.inUnit(SIMTIME_US)) / 1000000.0);

        if (state->max_bw > 0)
        {
            feature1 = double(state->avg_thr) / double(state->max_bw);
            feature2 = std::min((double)state->pacing_rate / (double)state->max_bw, 10.0);
            feature3 = 5.0 * loss_rate / (double)state->max_bw;
        }
        else
        {
            feature1 = feature2 = feature3 = 0;
        }

        feature4 = (double)(state->orca_cnt) / (double)(state->snd_cwnd / state->snd_mss);
        feature5 = (double)(simTime().inUnit(SIMTIME_US) - state->last_mi_t.inUnit(SIMTIME_US)) / 1000000.0;
        feature6 = (double)(state->min_rtt / 1000.0) / (double)(state->avg_urtt / 1000.0);

        double delay_metric;

        if ((state->min_rtt / 1000.0) * state->delay_margin_coef < (double)(state->avg_urtt) / 1000.0)
            delay_metric = ((double)state->min_rtt / 1000.0) * state->delay_margin_coef / ((double)(state->avg_urtt) / 1000.0);
        else
            delay_metric = 1.0;

        feature7 = delay_metric;

        conn->emit(feature1Signal, feature1);
        conn->emit(feature2Signal, feature2);
        conn->emit(feature3Signal, feature3);
        conn->emit(feature4Signal, feature4);
        conn->emit(feature5Signal, feature5);
        conn->emit(feature6Signal, feature6);
        conn->emit(feature7Signal, feature7);

        isValid = true;

        return {
            feature1,
            feature2,
            feature3,
            feature4,
            feature5,
            feature6,
            feature7};

        }else{
             // We increase the cwnd by 1.1x
        double cwnd = state->snd_cwnd / state->snd_mss;
        cwnd = (1100 * cwnd)/100;
        state->snd_cwnd = cwnd * state->snd_mss;
        state->last_mi_t = simTime();

        }
    }
    else
    {  
        isValid = false;
        return {
            0,
            0,
            0,
            0,
            0,
            0,
            0};
    }
}
RewardType Orca::computeReward()
{
    if (state->orca_cnt > 0 && !state->lossRecovery && state->snd_cwnd >= state->ssthresh)
    {
        double loss_rate = (double)(state->lost_bytes - state->pre_lost_bytes) / ((double)(simTime().inUnit(SIMTIME_US) - state->last_mi_t.inUnit(SIMTIME_US)) / 1000000.0);
        double delay_metric;

        if ((double)(state->min_rtt / 1000.0) * state->delay_margin_coef < (state->avg_urtt >> 3) / 1000.0)
            delay_metric = (state->min_rtt / 1000.0) * state->delay_margin_coef / ((double)state->avg_urtt / 1000.0);
        else
            delay_metric = 1.0;

        double reward = ((double)state->avg_thr - 5.0 * loss_rate) / (double)state->max_bw * delay_metric;

        conn->emit(rewardSignal, reward);
        return reward;
    }
    else
    {
        return 0;
    }
}

void Orca::cleanup() {}

void Orca::decisionMade(ActionType action)
{
    //Ignore if done
    if (!done)
    {
        uint32_t cwnd = state->snd_cwnd / state->snd_mss;

        if (state->snd_cwnd < state->ssthresh)
        {

        }
        else
        {
            uint32_t target_ratio = ((action * 100) * cwnd) / 100;
            conn->emit(actionSignal, action);

            state->snd_cwnd = std::max(target_ratio * state->snd_mss, state->snd_mss);
        }
        state->last_mi_t = simTime();
    }
}

ObsType Orca::getRLState() {}
RewardType Orca::getReward() {}
bool Orca::getDone() {}

#endif