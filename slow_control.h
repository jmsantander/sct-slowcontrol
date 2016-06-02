// slowcontrol.h
// Header file containing classes for CTA slow control software

#include <vector>

class Target_status
{
private:
    double voltage;
    double current;
    double temperature;
public:
    void set_voltage(double new_voltage);
    double get_voltage();
    void set_current(double new_current);
    double get_current();
    void set_temperature(double new_temperature);
    double get_temperature();
};

void Target_status::set_voltage(double new_voltage)
{
    voltage = new_voltage;
}

double Target_status::get_voltage()
{
    return voltage;
}

void Target_status::set_current(double new_current)
{
    current = new_current;
}

double Target_status::get_current()
{
    return current;
}

void Target_status::set_temperature(double new_temperature)
{
    temperature = new_temperature;
}

double Target_status::get_temperature()
{
    return temperature;
}

class Target_settings
{
    double desired;
    double min;
    double max;
    double critical_min;
    double critical_max;
public:
    void set_desired(double new_desired);
    double get_desired();
    void set_min(double new_min);
    double get_min();
    void set_max(double new_max);
    double get_max();
    void set_critical_min(double new_critical_min);
    double get_critical_min();
    void set_critical_max(double new_critical_max);
    double get_critical_max();
};

void Target_settings::set_desired(double new_desired)
{
    desired = new_desired;
}

double Target_settings::get_desired()
{
    return desired;
}

void Target_settings::set_min(double new_min)
{
    min = new_min;
}

double Target_settings::get_min()
{
    return min;
}
void Target_settings::set_max(double new_max)
{
    max = new_max;
}

double Target_settings::get_max()
{
    return max;
}
void Target_settings::set_critical_max(double new_critical_max)
{
    critical_max = new_critical_max;
}

double Target_settings::get_critical_max()
{
    return critical_max;
}
void Target_settings::set_critical_min(double new_critical_min)
{
    critical_min = new_critical_min;
}

double Target_settings::get_critical_min()
{
    return critical_min;
}

class Target
{
public:
    Target_status status;
    Target_settings settings;
};

class FEE
{
private:
    int n_targets;
public: 
    std::vector<Target> targets;
    FEE(int n_targets_initialization);
    int get_n_targets();
};

FEE::FEE(int n_targets_initialization)
{
    n_targets = n_targets_initialization;
    targets.reserve(n_targets);
    for (int i = 0; i < n_targets; i++)
    {
        targets.push_back(Target());
    }
}

int FEE::get_n_targets()
{
    return n_targets;
}

class Backplane
{
private:
    int n_fees;
    int n_targets;
public:
    std::vector<FEE> fees;
    Backplane(int n_fees, int n_targets);
    int get_n_fees();
    int get_n_targets();
};

Backplane::Backplane(int n_fees_initialization, int n_targets_initialization)
{
    n_fees = n_fees_initialization;
    n_targets = n_targets_initialization;
    fees.reserve(n_fees);
    for (int i = 0; i < n_fees; i++)
    {
        fees.push_back(FEE(n_targets));
    }
}

int Backplane::get_n_fees()
{
    return n_fees;
}

int Backplane::get_n_targets()
{
    return n_targets;
}
