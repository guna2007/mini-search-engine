#include "query/scorer.h"

scorer::scorer(const inverted_index& inv, const forward_index& fwd,
               const std::vector<std::string>& query_terms, bm25_params p)
    : fwd_idx(fwd), params(p) {

    float avgdl = static_cast<float>(inv.avg_doc_length);
    // guard against zero - happens with empty corpora
    if (avgdl < 1.0f) avgdl = 1.0f;

    term_scorers.reserve(query_terms.size());

    for (const auto& term : query_terms) {
        const auto* pl = inv.lookup(term);
        if (!pl) continue;

        term_scorer ts;
        ts.pl = pl;
        ts.idf = pl->idf;
        ts.k1_plus_1 = p.k1 + 1.0f;
        ts.k1_times_one_minus_b = p.k1 * (1.0f - p.b);
        ts.k1_times_b_over_avgdl = p.k1 * p.b / avgdl;

        term_scorers.push_back(ts);
    }
}

float scorer::score(uint32_t doc_id) const {
    const auto* doc = fwd_idx.get(doc_id);
    if (!doc) return 0.0f;
    return score_with_length(doc_id, static_cast<float>(doc->length));
}

float scorer::score_with_length(uint32_t doc_id, float doc_length) const {
    float total = 0.0f;

    for (const auto& ts : term_scorers) {
        const auto* p = ts.pl->find(doc_id);
        if (!p) continue;

        float tf = static_cast<float>(p->term_freq);

        // bm25: IDF * (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * dl / avgdl))
        // precomputed: k1_plus_1, k1_times_one_minus_b, k1_times_b_over_avgdl
        float numerator = tf * ts.k1_plus_1;
        float denominator = tf + ts.k1_times_one_minus_b + ts.k1_times_b_over_avgdl * doc_length;

        total += ts.idf * numerator / denominator;
    }

    return total;
}
