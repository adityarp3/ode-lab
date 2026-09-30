#include "odelab/io.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace odelab {

namespace {

std::string json_number(double v) {
    if (!std::isfinite(v)) return "null";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.17g", v);
    return std::string(buf);
}

std::string json_bool(bool b) { return b ? "true" : "false"; }

std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c;
        }
    }
    return out;
}

std::string json_string(const std::string& s) {
    return "\"" + json_escape(s) + "\"";
}

} // namespace

// ---------- CSV ----------

void write_csv(const std::string& path, const Trajectory& traj) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    size_t dim = traj.x.empty() ? 0 : traj.x[0].size();
    out << "t";
    for (size_t d = 0; d < dim; ++d) out << ",x" << d;
    out << "\n";

    for (size_t i = 0; i < traj.t.size(); ++i) {
        out << traj.t[i];
        for (size_t d = 0; d < dim; ++d) out << "," << traj.x[i][d];
        out << "\n";
    }
}

void write_csv(const std::string& path, const PhaseLineResult& pl) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    out << "x,xdot\n";
    for (size_t i = 0; i < pl.x.size(); ++i) {
        out << pl.x[i] << "," << pl.xdot[i] << "\n";
    }
}

void write_fixed_points_csv(const std::string& path,
                             const std::vector<FixedPoint>& fixed_points) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    out << "x,stability\n";
    for (const auto& fp : fixed_points) {
        out << fp.x << "," << to_string(fp.stability) << "\n";
    }
}

void write_csv(const std::string& path, const DirectionField& field) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    size_t dim = field.samples.empty() ? 0 : field.samples[0].vector.size();
    out << "axis1,axis2";
    for (size_t d = 0; d < dim; ++d) out << ",v" << d;
    out << "\n";

    for (const auto& s : field.samples) {
        out << s.position[0] << "," << s.position[1];
        for (size_t d = 0; d < dim; ++d) out << "," << s.vector[d];
        out << "\n";
    }
}

void write_csv(const std::string& path, const std::vector<IsoclinePoint>& points) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    out << "axis1,axis2\n";
    for (const auto& p : points) {
        out << p.axis1 << "," << p.axis2 << "\n";
    }
}

// ---------- JSON ----------

std::string to_json(const Trajectory& traj) {
    std::ostringstream out;
    out << "{\"t\":[";
    for (size_t i = 0; i < traj.t.size(); ++i) {
        if (i) out << ",";
        out << json_number(traj.t[i]);
    }
    out << "],\"x\":[";
    for (size_t i = 0; i < traj.x.size(); ++i) {
        if (i) out << ",";
        out << "[";
        for (size_t d = 0; d < traj.x[i].size(); ++d) {
            if (d) out << ",";
            out << json_number(traj.x[i][d]);
        }
        out << "]";
    }
    out << "],";
    out << "\"diverged\":" << json_bool(traj.diverged) << ",";
    out << "\"diverged_at_t\":" << (traj.diverged ? json_number(traj.diverged_at_t) : "null") << ",";
    out << "\"divergence_reason\":"
        << (traj.divergence_reason.empty() ? "null" : json_string(traj.divergence_reason));
    out << "}";
    return out.str();
}

std::string to_json(const std::vector<FixedPoint>& fixed_points) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < fixed_points.size(); ++i) {
        if (i) out << ",";
        out << "{\"x\":" << json_number(fixed_points[i].x)
            << ",\"stability\":" << json_string(to_string(fixed_points[i].stability)) << "}";
    }
    out << "]";
    return out.str();
}

std::string to_json(const PhaseLineResult& pl) {
    std::ostringstream out;
    out << "{\"x\":[";
    for (size_t i = 0; i < pl.x.size(); ++i) {
        if (i) out << ",";
        out << json_number(pl.x[i]);
    }
    out << "],\"xdot\":[";
    for (size_t i = 0; i < pl.xdot.size(); ++i) {
        if (i) out << ",";
        out << json_number(pl.xdot[i]);
    }
    out << "],\"fixed_points\":" << to_json(pl.fixed_points) << ",";
    out << "\"has_invalid_samples\":" << json_bool(pl.has_invalid_samples);
    out << "}";
    return out.str();
}

std::string to_json(const DirectionField& field) {
    std::ostringstream out;
    out << "{\"nx\":" << field.nx << ",\"ny\":" << field.ny << ",";
    out << "\"has_invalid_samples\":" << json_bool(field.has_invalid_samples) << ",";
    out << "\"samples\":[";
    for (size_t i = 0; i < field.samples.size(); ++i) {
        if (i) out << ",";
        const auto& s = field.samples[i];
        out << "{\"position\":[" << json_number(s.position[0]) << "," << json_number(s.position[1]) << "],";
        out << "\"vector\":[";
        for (size_t d = 0; d < s.vector.size(); ++d) {
            if (d) out << ",";
            out << json_number(s.vector[d]);
        }
        out << "]}";
    }
    out << "]}";
    return out.str();
}

std::string to_json(const std::vector<IsoclinePoint>& points) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < points.size(); ++i) {
        if (i) out << ",";
        out << "{\"axis1\":" << json_number(points[i].axis1)
            << ",\"axis2\":" << json_number(points[i].axis2) << "}";
    }
    out << "]";
    return out.str();
}

void write_json(const std::string& path, const Trajectory& traj) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);
    out << to_json(traj);
}

void write_json(const std::string& path, const PhaseLineResult& pl) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);
    out << to_json(pl);
}

void write_json(const std::string& path, const std::vector<FixedPoint>& fixed_points) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);
    out << to_json(fixed_points);
}

void write_json(const std::string& path, const DirectionField& field) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);
    out << to_json(field);
}

void write_json(const std::string& path, const std::vector<IsoclinePoint>& points) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);
    out << to_json(points);
}

} // namespace odelab
