module;

export module Karm.Core:glob.fuzzy;

import :base.vec;
import :base.ctype;
import :base.string;

namespace Karm::Glob {

export struct Match {
    int score = 0;
    Vec<urange> ranges = {};
};

static bool _isBoundary(Str text, usize idx) {
    if (idx == 0)
        return true;

    char c = text[idx];
    char prev = text[idx - 1];

    if (prev == '_' or prev == ' ' or prev == '-' or prev == '/' or prev == '.')
        return true;

    if (isAsciiUpper(c) and isAsciiLower(prev))
        return true;

    return false;
}

static int _calculateScore(Str text, Vec<usize> const& indices) {
    if (indices.len() == 0)
        return 0;

    int score = 0;
    int consecutiveCount = 0;

    for (usize k = 0; k < indices.len(); k++) {
        usize idx = indices[k];

        bool isStart = (idx == 0);
        bool isWordBoundary = _isBoundary(text, idx);
        bool isConsecutive = (k > 0 and idx == indices[k - 1] + 1);

        if (isStart) {
            score += 100;
        } else if (isWordBoundary) {
            score += 80;
        } else if (isConsecutive) {
            score += 40;
            score += (consecutiveCount * 5);
        } else {
            score += 10;
        }

        if (k > 0) {
            usize gap = idx - indices[k - 1] - 1;
            if (gap > 0) {
                score -= (gap > 10 ? 10 : gap);
                consecutiveCount = 0;
            } else {
                consecutiveCount++;
            }
        }
    }
    return score;
}

static bool _findBestMatch(Str text, Str pattern, usize ti, usize pi, Vec<usize>& outIndices) {
    if (pi == pattern.len())
        return true;
    if (ti == text.len())
        return false;

    for (usize i = ti; i < text.len(); i++) {
        if (toAsciiLower(text[i]) == toAsciiLower(pattern[pi])) {
            bool isCurrentBoundary = _isBoundary(text, i);

            if (!isCurrentBoundary) {
                for (usize j = i + 1; j < text.len(); j++) {
                    if (toAsciiLower(text[j]) == toAsciiLower(pattern[pi]) and _isBoundary(text, j)) {
                        Vec<usize> betterPath;
                        if (_findBestMatch(text, pattern, j + 1, pi + 1, betterPath)) {
                            outIndices.pushBack(j);
                            for (usize idx : betterPath)
                                outIndices.pushBack(idx);
                            return true;
                        }
                        break;
                    }
                }
            }

            Vec<usize> subPath;
            if (_findBestMatch(text, pattern, i + 1, pi + 1, subPath)) {
                outIndices.pushBack(i);
                for (usize idx : subPath)
                    outIndices.pushBack(idx);
                return true;
            }
        }
    }

    return false;
}

export Opt<Match> matchFuzzy(Str text, Str pattern) {
    if (pattern.len() == 0)
        return NONE;

    Vec<usize> indices;

    if (!_findBestMatch(text, pattern, 0, 0, indices))
        return NONE;

    Match result;
    result.score = _calculateScore(text, indices);

    if (indices.len() > 0) {
        usize start = indices[0];
        usize len = 1;
        for (usize k = 1; k < indices.len(); k++) {
            if (indices[k] == indices[k - 1] + 1) {
                len++;
            } else {
                result.ranges.pushBack(urange{start, len});
                start = indices[k];
                len = 1;
            }
        }
        result.ranges.pushBack(urange{start, len});
    }

    return result;
}

} // namespace Karm::Glob
