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
private:
    double voltage_desired;
    double voltage_min;
    double voltage_max;
    double voltage_critical_min;
    double voltage_critical_max;

    double current_desired;
    double current_min;
    double current_max;
    double current_critical_min;
    double current_critical_max;

    double temperature_desired;
    double temperature_min;
    double temperature_max;
    double temperature_critical_min;
    double temperature_critical_max;
public:
    void set_voltage_desired(double new_voltage_desired);
    double get_voltage_desired();
    void set_voltage_min(double new_voltage_min);
    double get_voltage_min();
    void set_voltage_max(double new_voltage_max);
    double get_voltage_max();
    void set_voltage_critical_min(double new_voltage_critical_min);
    double get_voltage_critical_min();
    void set_voltage_critical_max(double new_voltage_critical_max);
    double get_voltage_critical_max();

    void set_current_desired(double new_current_desired);
    double get_current_desired();
    void set_current_min(double new_current_min);
    double get_current_min();
    void set_current_max(double new_current_max);
    double get_current_max();
    void set_current_critical_min(double new_current_critical_min);
    double get_current_critical_min();
    void set_current_critical_max(double new_current_critical_max);
    double get_current_critical_max();

    void set_temperature_desired(double new_temperature_desired);
    double get_temperature_desired();
    void set_temperature_min(double new_temperature_min);
    double get_temperature_min();
    void set_temperature_max(double new_temperature_max);
    double get_temperature_max();
    void set_temperature_critical_min(double new_temperature_critical_min);
    double get_temperature_critical_min();
    void set_temperature_critical_max(double new_temperature_critical_max);
    double get_temperature_critical_max();
};

// Voltage getters and setters

void Target_settings::set_voltage_desired(double new_voltage_desired)
{
    voltage_desired = new_voltage_desired;
}

double Target_settings::get_voltage_desired()
{
    return voltage_desired;
}

void Target_settings::set_voltage_min(double new_voltage_min)
{
    voltage_min = new_voltage_min;
}

double Target_settings::get_voltage_min()
{
    return voltage_min;
}

void Target_settings::set_voltage_max(double new_voltage_max)
{
    voltage_max = new_voltage_max;
}

double Target_settings::get_voltage_max()
{
    return voltage_max;
}

void Target_settings::set_voltage_critical_min(double new_voltage_critical_min)
{
    voltage_critical_min = new_voltage_critical_min;
}

double Target_settings::get_voltage_critical_min()
{
    return voltage_critical_min;
}

void Target_settings::set_voltage_critical_max(double new_voltage_critical_max)
{
    voltage_critical_max = new_voltage_critical_max;
}

double Target_settings::get_voltage_critical_max()
{
    return voltage_critical_max;
}

// Current getters and setters

void Target_settings::set_current_desired(double new_current_desired)
{
    current_desired = new_current_desired;
}

double Target_settings::get_current_desired()
{
    return current_desired;
}

void Target_settings::set_current_min(double new_current_min)
{
    current_min = new_current_min;
}

double Target_settings::get_current_min()
{
    return current_min;
}

void Target_settings::set_current_max(double new_current_max)
{
    current_max = new_current_max;
}

double Target_settings::get_current_max()
{
    return current_max;
}

void Target_settings::set_current_critical_min(double new_current_critical_min)
{
    current_critical_min = new_current_critical_min;
}

double Target_settings::get_current_critical_min()
{
    return current_critical_min;
}

void Target_settings::set_current_critical_max(double new_current_critical_max)
{
    current_critical_max = new_current_critical_max;
}

double Target_settings::get_current_critical_max()
{
    return current_critical_max;
}

// Temperature getters and setters

void Target_settings::set_temperature_desired(double new_temperature_desired)
{
    temperature_desired = new_temperature_desired;
}

double Target_settings::get_temperature_desired()
{
    return temperature_desired;
}

void Target_settings::set_temperature_min(double new_temperature_min)
{
    temperature_min = new_temperature_min;
}

double Target_settings::get_temperature_min()
{
    return temperature_min;
}

void Target_settings::set_temperature_max(double new_temperature_max)
{
    temperature_max = new_temperature_max;
}

double Target_settings::get_temperature_max()
{
    return temperature_max;
}

void Target_settings::set_temperature_critical_max(double new_temperature_critical_max)
{
    temperature_critical_max = new_temperature_critical_max;
}

double Target_settings::get_temperature_critical_max()
{
    return temperature_critical_max;
}

void Target_settings::set_temperature_critical_min(double new_temperature_critical_min)
{
    temperature_critical_min = new_temperature_critical_min;
}

double Target_settings::get_temperature_critical_min()
{
    return temperature_critical_min;
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
