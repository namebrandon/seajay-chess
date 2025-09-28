#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/board.h"
#include "core/magic_bitboards.h"
#include "evaluation/evaluate.h"
#include "evaluation/pawn_structure.h"
#include "evaluation/types.h"
#include "search/lmr.h"

using namespace seajay;
using namespace seajay::eval;

namespace {

std::filesystem::path resolveFromSourceRoot(const std::string& relativePath);

struct EvalExpectation {
    int minCp{};
    int maxCp{};
    int extraToleranceCp{0};
    bool enforce{false};
};

struct PositionSample {
    std::string fen;
    int evalCp{};
    std::optional<EvalExpectation> expectation;
    bool withinRange{true};
    int appliedToleranceCp{0};
    bool enforce{false};
    bool countsTowardFailure{false};
};

std::string trim(std::string_view input) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    size_t start = 0;
    while (start < input.size() && isSpace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start && isSpace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return std::string(input.substr(start, end - start));
}

bool looksLikeFEN(const std::string& line) {
    if (line.find('/') == std::string::npos) {
        return false;
    }
    std::istringstream iss(line);
    std::string token;
    int fields = 0;
    std::array<std::string, 6> tokens{};
    while (iss >> token) {
        if (fields < 6) {
            tokens[fields] = token;
        }
        ++fields;
    }
    if (fields != 6) {
        return false;
    }

    const auto& pieces = tokens[0];
    const auto isValidPieceChar = [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) ||
               ch == '/' ||
               std::string_view("prnbqkPRNBQK").find(ch) != std::string_view::npos;
    };
    return std::all_of(pieces.begin(), pieces.end(), isValidPieceChar);
}

std::vector<std::string> loadFENs(const std::string& relativePath) {
    const auto path = resolveFromSourceRoot(relativePath);
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open problem positions file: " + path.string());
    }

    std::vector<std::string> fens;
    std::string rawLine;
    while (std::getline(input, rawLine)) {
        const std::string line = trim(rawLine);
        if (line.empty()) {
            continue;
        }
        if (looksLikeFEN(line)) {
            fens.push_back(line);
        }
    }
    return fens;
}

std::filesystem::path resolveFromSourceRoot(const std::string& relativePath) {
    namespace fs = std::filesystem;

#ifdef SEAJAY_SOURCE_DIR
    static const fs::path sourceRoot(SEAJAY_SOURCE_DIR);
    if (!sourceRoot.empty()) {
        fs::path resolved = sourceRoot / relativePath;
        if (fs::exists(resolved)) {
            return resolved;
        }
    }
#endif

    return fs::path(relativePath);
}

std::unordered_map<std::string, EvalExpectation> loadExpectations(const std::string& relativePath) {
    namespace fs = std::filesystem;

    const fs::path path = resolveFromSourceRoot(relativePath);
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open expectations file: " + path.string());
    }

    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::unordered_map<std::string, EvalExpectation> expectations;

    std::regex entryRegex("\\{\\s*\"fen\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"min_cp\"\\s*:\\s*(-?\\d+)\\s*,\\s*\"max_cp\"\\s*:\\s*(-?\\d+)(?:\\s*,\\s*\"extra_tolerance_cp\"\\s*:\\s*(\\d+))?(?:\\s*,\\s*\"enforce\"\\s*:\\s*(true|false))?(?:\\s*,\\s*\"notes\"\\s*:\\s*\"[^\"]*\")?\\s*\\}");

    size_t parsedEntries = 0;
    for (std::sregex_iterator it(content.begin(), content.end(), entryRegex), end; it != end; ++it) {
        ++parsedEntries;
        const std::smatch& match = *it;

        EvalExpectation expectation;
        expectation.minCp = std::stoi(match[2]);
        expectation.maxCp = std::stoi(match[3]);
        if (match[4].matched) {
            expectation.extraToleranceCp = std::stoi(match[4]);
        }
        if (match[5].matched) {
            expectation.enforce = (match[5] == "true");
        }

        expectations.emplace(match[1], expectation);
    }

    if (parsedEntries == 0) {
        throw std::runtime_error("No expectations parsed from: " + path.string());
    }

    return expectations;
}

struct HarnessOptions {
    int toleranceCp = 50;
    bool failOnOutOfRange = false;
    bool verbose = false;
    std::string outputPath;
    std::string expectationsPath = "tests/eval/problem_position_expectations.json";
};

HarnessOptions parseOptions(int argc, char** argv) {
    HarnessOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--tolerance" && i + 1 < argc) {
            opts.toleranceCp = std::stoi(argv[++i]);
        } else if (arg == "--fail-on-out-of-range") {
            opts.failOnOutOfRange = true;
        } else if (arg == "--output" && i + 1 < argc) {
            opts.outputPath = argv[++i];
        } else if (arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--expectations" && i + 1 < argc) {
            opts.expectationsPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: test_eval_problem_positions [options]\n"
                      << "  --tolerance <cp>          Additional slack applied to reference ranges (default 50).\n"
                      << "  --output <path>           Write CSV baseline to path.\n"
                      << "  --expectations <path>     Override expectations JSON (default: tests/eval/problem_position_expectations.json).\n"
                      << "  --fail-on-out-of-range    Exit non-zero when SeaJay score exceeds reference range.\n"
                      << "  --verbose                 Print per-position evaluation details.\n";
            std::exit(0);
        }
    }
    return opts;
}

void writeCsvHeader(std::ofstream& out) {
    out << "fen,eval_cp,ref_min_cp,ref_max_cp,tolerance_cp,lower_bound_cp,upper_bound_cp,within_range,enforce\n";
}

void writeCsvRow(std::ofstream& out, const PositionSample& sample, int toleranceCp) {
    out << '"' << sample.fen << '"' << ','
        << sample.evalCp;
    if (sample.expectation) {
        const auto& exp = *sample.expectation;
        out << ',' << exp.minCp << ',' << exp.maxCp << ','
            << toleranceCp << ','
            << exp.minCp - toleranceCp << ',' << exp.maxCp + toleranceCp << ','
            << (sample.withinRange ? "true" : "false") << ','
            << (sample.enforce ? "true" : "false");
    } else {
        out << ",,,,,,";
    }
    out << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        const HarnessOptions options = parseOptions(argc, argv);

        const auto expectations = loadExpectations(options.expectationsPath);
        const auto fens = loadFENs("external/problem_positions.txt");

        seajay::magic_v2::initMagics();
        PawnStructure::initPassedPawnMasks();
        search::initLMRTable();

        std::ofstream csv;
        if (!options.outputPath.empty()) {
            csv.open(options.outputPath);
            if (!csv) {
                std::cerr << "Failed to open output path: " << options.outputPath << std::endl;
                return 1;
            }
            writeCsvHeader(csv);
        }

        std::vector<PositionSample> samples;
        samples.reserve(fens.size());

        for (const auto& fen : fens) {
            Board board;
            if (!board.fromFEN(fen)) {
                std::cerr << "[ERROR] Failed to parse FEN: " << fen << std::endl;
                if (options.failOnOutOfRange) {
                    return 1;
                }
                continue;
            }
            board.clearGameHistory();

            const Score score = eval::evaluate(board);
            PositionSample sample;
            sample.fen = fen;
            sample.evalCp = static_cast<int>(score.to_cp());

            if (auto it = expectations.find(fen); it != expectations.end()) {
                sample.expectation = it->second;
                sample.enforce = it->second.enforce;
                sample.appliedToleranceCp = options.toleranceCp + it->second.extraToleranceCp;
                const int lower = it->second.minCp - sample.appliedToleranceCp;
                const int upper = it->second.maxCp + sample.appliedToleranceCp;
                sample.withinRange = sample.evalCp >= lower && sample.evalCp <= upper;
                sample.countsTowardFailure = sample.enforce && !sample.withinRange;
            } else {
                sample.expectation = std::nullopt;
                sample.withinRange = true;
                sample.appliedToleranceCp = options.toleranceCp;
                sample.enforce = false;
                sample.countsTowardFailure = false;
            }

            samples.emplace_back(sample);
            if (csv.is_open()) {
                writeCsvRow(csv, sample, sample.appliedToleranceCp);
            }
        }

        size_t outOfRangeCount = 0;
        for (const auto& sample : samples) {
            if (sample.countsTowardFailure) {
                ++outOfRangeCount;
            }

            const bool shouldReport = options.verbose || sample.countsTowardFailure || (!sample.withinRange && !sample.expectation);
            if (shouldReport) {
                std::string tag;
                if (!sample.expectation) {
                    tag = sample.withinRange ? "[INFO]" : "[WARN]";
                } else if (sample.withinRange) {
                    tag = sample.enforce ? "[OK]" : "[OBS]";
                } else {
                    tag = sample.enforce ? "[WARN]" : "[TODO]";
                }

                std::cout << tag << ' ' << sample.fen << " => " << sample.evalCp << " cp";
                if (sample.expectation) {
                    const auto& exp = *sample.expectation;
                    std::cout << " (reference " << exp.minCp << ".." << exp.maxCp
                              << ", tolerance ±" << sample.appliedToleranceCp
                              << ", enforce=" << (sample.enforce ? "true" : "false") << ')';
                }
                std::cout << std::endl;
            }
        }

        std::cout << "\nAnalyzed " << samples.size() << " positions" << std::endl;
        std::cout << "Out-of-range evaluations (enforced): " << outOfRangeCount << std::endl;

        if (outOfRangeCount > 0 && options.failOnOutOfRange) {
            return 1;
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled exception: " << ex.what() << std::endl;
        return 1;
    }
}
