//
// Created by liyinbin on 2020/2/8.
//



#include <abel/metrics/prom_serializer.h>
#include <cmath>
#include <limits>
#include <locale>
#include <ostream>


namespace abel {
    namespace metrics {

// Write a double as a string, with proper formatting for infinity and NaN
        void WriteValue(std::ostream &out, double value) {
            if (std::isnan(value)) {
                out << "Nan";
            } else if (std::isinf(value)) {
                out << (value < 0 ? "-Inf" : "+Inf");
            } else {
                auto saved_flags = out.setf(std::ios::fixed, std::ios::floatfield);
                out << value;
                out.setf(saved_flags, std::ios::floatfield);
            }
        }

        void WriteValue(std::ostream &out, const std::string &value) {
            for (auto c : value) {
                if (c == '\\' || c == '"' || c == '\n') {
                    out << "\\";
                }
                out << c;
            }
        }

// Write a line header: metric name and labels
        template<typename T = std::string>
        void WriteHead(std::ostream &out, const std::string &name, const cache_metrics &metric,
                       const std::string &suffix = "",
                       const std::string &extraLabelName = "",
                       const T &extraLabelValue = T()) {
            out << name << suffix;
            if (!metric.family->tags.empty() || !extraLabelName.empty()) {
                out << "{";
                const char *prefix = "";

                for (auto it = metric.family->tags.begin(); it != metric.family->tags.end(); it++) {
                    out << prefix << it->first << "=\"";
                    WriteValue(out, it->second);
                    out << "\"";
                    prefix = ",";
                }
                if (!extraLabelName.empty()) {
                    out << prefix << extraLabelName << "=\"";
                    WriteValue(out, extraLabelValue);
                    out << "\"";
                }
                out << "}";
            }
            out << " ";
        }

// Write a line trailer: timestamp
        void WriteTail(std::ostream &out, const cache_metrics &metrics) {
            if (metrics.timestamp_ms != 0) {
                out << " " << metrics.timestamp_ms;
            }
            out << "\n";
        }

        void SerializeCounter(std::ostream &out, const std::string &name, const cache_metrics &metric) {
            WriteHead(out, name, metric);
            WriteValue(out, metric.counter.value);
            WriteTail(out, metric);
        }

        void SerializeGauge(std::ostream &out, const std::string &name,
                            const cache_metrics &metric) {
            WriteHead(out, name, metric);
            WriteValue(out, metric.gauge.value);
            WriteTail(out, metric);
        }

        void SerializeHistogram(std::ostream &out, const std::string &name,
                                const cache_metrics &metric) {
            auto &hist = metric.histogram;
            WriteHead(out, name, metric, "_count");
            out << hist.sample_count;
            WriteTail(out, metric);

            WriteHead(out, name, metric, "_sum");
            WriteValue(out, hist.sample_sum);
            WriteTail(out, metric);

            double last = -std::numeric_limits<double>::infinity();
            for (auto &b : hist.bucket) {
                WriteHead(out, name, metric, "_bucket", "le", b.upper_bound);
                last = b.upper_bound;
                out << b.cumulative_count;
                WriteTail(out, metric);
            }

            if (last != std::numeric_limits<double>::infinity()) {
                WriteHead(out, name, metric, "_bucket", "le", "+Inf");
                out << hist.sample_count;
                WriteTail(out, metric);
            }
        }

        std::string MakeName(const cache_metrics &metrics) {
            std::string ret;
            ret = metrics.family->prefix;
            ret += metrics.family->separator;
            ret += metrics.name;
            return ret;
        }

        void SerializeMetrics(std::ostream &out, const cache_metrics &metrics) {

            std::string metricsName = MakeName(metrics);
            out << "# HELP " << metricsName << "\n";

            switch (metrics.type) {
                case metrics_type::mt_counter:
                    out << "# TYPE " << metricsName << " counter\n";
                    SerializeCounter(out, metricsName, metrics);
                    break;
                case metrics_type::mt_gauge:
                    out << "# TYPE " << metricsName << " gauge\n";
                    SerializeGauge(out, metricsName, metrics);
                    break;
                case metrics_type::mt_histogram:
                    out << "# TYPE " << metricsName << " histogram\n";
                    SerializeHistogram(out, metricsName, metrics);
                    break;
                case metrics_type::mt_timer:
                    out << "# TYPE " << metricsName << " histogram\n";
                    SerializeHistogram(out, metricsName, metrics);
                    break;
                default:
                    break;
            }
        }

        void prometheus_serializer::format(std::ostream &out,
                                           const std::vector<cache_metrics> &metrics) const {
            for (auto &metric : metrics) {
                SerializeMetrics(out, metric);
            }
        }

    }  //namespace metrics
} //namespcace abel
