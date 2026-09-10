#!/usr/bin/env python3
"""Generate a rich, diverse corpus in JSONL format for the search engine."""

import json, random, os

random.seed(42)

topics = {
    "computer_science": {
        "terms": [
            "algorithm", "data structure", "binary tree", "hash table", "graph",
            "sorting", "searching", "complexity", "recursion", "dynamic programming",
            "cache", "memory", "processor", "compiler", "interpreter", "runtime",
            "stack", "queue", "linked list", "array", "pointer", "reference",
            "garbage collection", "virtual memory", "operating system", "kernel",
            "thread", "process", "mutex", "semaphore", "deadlock", "concurrency",
            "database", "sql", "indexing", "transaction", "replication", "sharding",
            "machine learning", "neural network", "deep learning", "gradient descent",
            "backpropagation", "convolution", "transformer", "attention mechanism",
            "distributed system", "consensus", "raft", "paxos", "load balancer",
            "microservice", "container", "kubernetes", "docker", "serverless",
            "encryption", "authentication", "firewall", "packet", "tcp", "udp",
            "socket", "protocol", "bandwidth", "latency", "throughput", "pipeline",
            "version control", "git", "continuous integration", "deployment",
            "abstraction", "polymorphism", "inheritance", "encapsulation",
            "functional programming", "lambda", "closure", "monad", "type system",
            "regular expression", "finite automaton", "turing machine", "computability",
            "binary search", "breadth first search", "depth first search",
            "dijkstra", "minimum spanning tree", "topological sort",
        ],
    },
    "physics": {
        "terms": [
            "quantum mechanics", "relativity", "thermodynamics", "electromagnetism",
            "particle physics", "wave function", "energy", "momentum", "force",
            "acceleration", "velocity", "mass", "photon", "electron", "proton",
            "neutron", "gravity", "magnetic field", "electric field", "radiation",
            "spectrum", "wavelength", "frequency", "amplitude", "interference",
            "dark matter", "dark energy", "black hole", "supernova", "neutron star",
            "quark", "boson", "fermion", "higgs boson", "standard model",
            "string theory", "quantum entanglement", "superposition", "decoherence",
            "nuclear fission", "nuclear fusion", "plasma", "semiconductor",
            "superconductor", "laser", "optics", "refraction", "diffraction",
            "doppler effect", "entropy", "heat transfer", "conduction", "convection",
            "fluid dynamics", "viscosity", "turbulence", "aerodynamics",
            "general relativity", "special relativity", "spacetime", "lorentz",
            "maxwell equations", "coulomb", "faraday", "electromagnetic induction",
        ],
    },
    "mathematics": {
        "terms": [
            "calculus", "linear algebra", "topology", "number theory", "statistics",
            "probability", "differential equation", "integral", "matrix", "vector",
            "eigenvalue", "polynomial", "prime number", "group theory", "ring theory",
            "measure theory", "functional analysis", "combinatorics", "graph theory",
            "optimization", "convex analysis", "numerical methods", "approximation",
            "fourier transform", "laplace transform", "taylor series", "convergence",
            "manifold", "riemann", "hilbert space", "banach space", "tensor",
            "stochastic process", "markov chain", "bayesian inference", "regression",
            "hypothesis testing", "normal distribution", "poisson distribution",
            "game theory", "nash equilibrium", "linear programming", "simplex method",
            "boolean algebra", "set theory", "zorn lemma", "axiom of choice",
            "fibonacci", "golden ratio", "fractal", "chaos theory", "dynamical system",
            "cryptography", "modular arithmetic", "elliptic curve", "galois theory",
        ],
    },
    "biology": {
        "terms": [
            "evolution", "genetics", "cell biology", "molecular biology", "ecology",
            "dna", "rna", "protein", "enzyme", "metabolism", "photosynthesis",
            "mitosis", "meiosis", "chromosome", "gene expression", "mutation",
            "natural selection", "biodiversity", "ecosystem", "population dynamics",
            "neuroscience", "immunology", "microbiology", "virology",
            "antibiotic", "vaccine", "pathogen", "bacteria", "virus", "fungus",
            "stem cell", "cloning", "crispr", "gene editing", "genomics",
            "proteomics", "bioinformatics", "phylogenetics", "taxonomy",
            "symbiosis", "parasitism", "predation", "food chain", "trophic level",
            "respiration", "fermentation", "atp", "mitochondria", "chloroplast",
            "nervous system", "synapse", "neurotransmitter", "dopamine", "serotonin",
            "hormone", "endocrine system", "insulin", "cortisol", "adrenaline",
            "organ transplant", "blood type", "hemoglobin", "platelet", "antibody",
        ],
    },
    "chemistry": {
        "terms": [
            "periodic table", "element", "compound", "molecule", "atom",
            "chemical bond", "covalent bond", "ionic bond", "hydrogen bond",
            "organic chemistry", "inorganic chemistry", "polymer", "catalyst",
            "oxidation", "reduction", "acid", "base", "ph", "buffer solution",
            "electrolysis", "electrochemistry", "galvanic cell", "battery",
            "crystal", "alloy", "ceramic", "nanomaterial", "carbon nanotube",
            "benzene", "ethanol", "methane", "ammonia", "sulfuric acid",
            "titration", "spectroscopy", "chromatography", "mass spectrometry",
            "thermochemistry", "enthalpy", "gibbs energy", "equilibrium",
            "reaction kinetics", "activation energy", "arrhenius equation",
            "isotope", "radioactive decay", "half life", "nuclear chemistry",
            "pharmaceutical", "drug design", "molecular docking", "synthesis",
        ],
    },
    "medicine": {
        "terms": [
            "diagnosis", "treatment", "surgery", "anesthesia", "radiology",
            "cardiology", "oncology", "neurology", "dermatology", "pediatrics",
            "psychiatry", "orthopedics", "ophthalmology", "gastroenterology",
            "pulmonology", "nephrology", "endocrinology", "hematology",
            "clinical trial", "placebo", "randomized controlled trial",
            "epidemiology", "pandemic", "endemic", "quarantine", "contact tracing",
            "blood pressure", "cholesterol", "diabetes", "hypertension", "stroke",
            "heart attack", "cancer", "tumor", "chemotherapy", "radiation therapy",
            "mri", "ct scan", "ultrasound", "biopsy", "stethoscope",
            "penicillin", "aspirin", "ibuprofen", "morphine", "antihistamine",
            "allergy", "asthma", "arthritis", "alzheimer", "parkinson",
            "physical therapy", "rehabilitation", "prosthetic", "telemedicine",
        ],
    },
    "astronomy": {
        "terms": [
            "solar system", "planet", "star", "galaxy", "nebula", "comet",
            "asteroid", "meteor", "constellation", "milky way", "andromeda",
            "telescope", "observatory", "hubble", "james webb", "radio telescope",
            "exoplanet", "habitable zone", "red dwarf", "white dwarf", "pulsar",
            "quasar", "magnetar", "cosmic ray", "gravitational wave",
            "big bang", "cosmic microwave background", "redshift", "expansion",
            "mars", "jupiter", "saturn", "venus", "mercury", "neptune", "uranus",
            "moon", "lunar eclipse", "solar eclipse", "tidal force",
            "space station", "satellite", "orbit", "rocket", "spacecraft",
            "astronaut", "space exploration", "nasa", "spacex", "launch vehicle",
            "light year", "parsec", "astronomical unit", "celestial mechanics",
        ],
    },
    "history": {
        "terms": [
            "ancient civilization", "medieval period", "renaissance", "industrial revolution",
            "world war", "cold war", "colonialism", "democracy", "monarchy", "empire",
            "trade route", "cultural exchange", "political reform", "social movement",
            "archaeological discovery", "historical document", "oral tradition",
            "economic development", "military strategy", "diplomatic relations",
            "roman empire", "greek philosophy", "egyptian pyramid", "silk road",
            "french revolution", "american revolution", "russian revolution",
            "ottoman empire", "byzantine empire", "mongol empire", "persian empire",
            "feudalism", "magna carta", "constitution", "declaration of independence",
            "abolition", "suffrage", "civil rights", "apartheid", "decolonization",
            "printing press", "gunpowder", "compass", "navigation", "cartography",
            "archaeology", "anthropology", "paleontology", "fossil", "artifact",
        ],
    },
    "economics": {
        "terms": [
            "supply", "demand", "inflation", "deflation", "recession",
            "gross domestic product", "unemployment", "interest rate", "central bank",
            "fiscal policy", "monetary policy", "trade deficit", "tariff", "subsidy",
            "stock market", "bond", "equity", "derivative", "hedge fund",
            "venture capital", "startup", "initial public offering", "valuation",
            "microeconomics", "macroeconomics", "behavioral economics", "econometrics",
            "market failure", "externality", "public good", "monopoly", "oligopoly",
            "price elasticity", "marginal cost", "opportunity cost", "comparative advantage",
            "globalization", "free trade", "protectionism", "world trade organization",
            "cryptocurrency", "blockchain", "digital currency", "fintech",
            "insurance", "mortgage", "credit score", "bankruptcy", "audit",
        ],
    },
    "literature": {
        "terms": [
            "novel", "poetry", "drama", "short story", "essay", "memoir",
            "fiction", "nonfiction", "narrative", "protagonist", "antagonist",
            "metaphor", "simile", "allegory", "symbolism", "irony", "satire",
            "tragedy", "comedy", "epic", "sonnet", "haiku", "ballad",
            "shakespeare", "homer", "tolstoy", "dostoevsky", "kafka", "orwell",
            "magical realism", "modernism", "postmodernism", "romanticism",
            "gothic literature", "science fiction", "fantasy", "dystopia",
            "literary criticism", "hermeneutics", "structuralism", "deconstruction",
            "publishing", "manuscript", "bestseller", "book club", "library",
            "translation", "bilingual", "dialect", "vernacular", "linguistics",
        ],
    },
    "philosophy": {
        "terms": [
            "epistemology", "ontology", "metaphysics", "ethics", "aesthetics",
            "logic", "existentialism", "phenomenology", "pragmatism", "stoicism",
            "utilitarianism", "deontology", "virtue ethics", "social contract",
            "free will", "determinism", "consciousness", "mind body problem",
            "plato", "aristotle", "kant", "nietzsche", "descartes", "hegel",
            "empiricism", "rationalism", "skepticism", "relativism", "nihilism",
            "political philosophy", "justice", "liberty", "equality", "rights",
            "philosophy of science", "falsifiability", "paradigm shift", "reductionism",
            "philosophy of mind", "qualia", "intentionality", "functionalism",
            "moral philosophy", "bioethics", "environmental ethics", "animal rights",
        ],
    },
    "geography": {
        "terms": [
            "continent", "ocean", "mountain", "river", "desert", "rainforest",
            "tundra", "savanna", "glacier", "volcano", "earthquake", "tsunami",
            "climate", "weather", "atmosphere", "precipitation", "humidity",
            "latitude", "longitude", "equator", "meridian", "time zone",
            "tectonic plate", "erosion", "sedimentation", "geomorphology",
            "cartography", "topography", "elevation", "bathymetry",
            "urbanization", "migration", "population density", "megalopolis",
            "deforestation", "desertification", "coral reef", "wetland",
            "renewable energy", "solar panel", "wind turbine", "hydroelectric",
            "carbon footprint", "greenhouse gas", "ozone layer", "climate change",
            "biodiversity hotspot", "national park", "conservation", "sustainability",
        ],
    },
    "music": {
        "terms": [
            "melody", "harmony", "rhythm", "tempo", "pitch", "timbre",
            "symphony", "concerto", "sonata", "opera", "jazz", "blues",
            "rock", "hip hop", "electronic music", "classical music", "folk music",
            "guitar", "piano", "violin", "drums", "saxophone", "flute", "cello",
            "composer", "conductor", "orchestra", "choir", "ensemble",
            "music theory", "chord progression", "key signature", "time signature",
            "improvisation", "counterpoint", "fugue", "aria", "libretto",
            "recording studio", "mixing", "mastering", "synthesizer", "sampler",
            "beethoven", "mozart", "bach", "coltrane", "miles davis",
            "album", "single", "concert", "festival", "tour",
        ],
    },
    "engineering": {
        "terms": [
            "civil engineering", "mechanical engineering", "electrical engineering",
            "structural analysis", "bridge", "skyscraper", "dam", "tunnel",
            "circuit", "transistor", "capacitor", "resistor", "inductor",
            "signal processing", "control system", "feedback loop", "pid controller",
            "robotics", "actuator", "sensor", "lidar", "computer vision",
            "materials science", "composite", "fatigue", "stress", "strain",
            "thermodynamics", "heat engine", "refrigeration", "hvac",
            "aerospace", "aircraft", "propulsion", "wind tunnel", "aerodynamics",
            "manufacturing", "cnc", "3d printing", "injection molding", "welding",
            "quality control", "tolerance", "calibration", "metrology",
            "renewable energy", "power grid", "smart grid", "energy storage",
            "biomedical engineering", "medical device", "implant", "biomechanics",
        ],
    },
    "psychology": {
        "terms": [
            "cognition", "perception", "attention", "memory", "learning",
            "intelligence", "creativity", "motivation", "emotion", "personality",
            "cognitive psychology", "behavioral psychology", "developmental psychology",
            "social psychology", "clinical psychology", "neuropsychology",
            "psychotherapy", "cognitive behavioral therapy", "counseling",
            "anxiety", "depression", "ptsd", "bipolar disorder", "schizophrenia",
            "attachment theory", "maslow", "freud", "jung", "skinner", "pavlov",
            "conditioning", "reinforcement", "punishment", "habituation",
            "working memory", "long term memory", "implicit memory", "amnesia",
            "confirmation bias", "cognitive dissonance", "groupthink", "conformity",
            "self esteem", "resilience", "mindfulness", "meditation", "wellbeing",
            "iq test", "personality test", "rorschach", "psychometrics",
        ],
    },
}

templates = [
    "In the field of {domain}, {t1} is a fundamental concept related to {t2}. The study of {t1} involves understanding how {t3} can be applied to solve problems efficiently. Modern research often leverages {t4} to achieve better outcomes. Experts continue to investigate the relationship between {t1} and {t2}, revealing new insights that challenge traditional understanding.",
    "The {t1} is widely studied in {domain} for tasks involving {t2}. When combined with {t3}, it provides an effective approach to {t4}. Research has shown that {t1} performs well under most practical conditions. The interplay between {t3} and {t4} continues to drive innovation in this area.",
    "{t1} was first described in the context of {t2} research. It relies on principles from {t3} and has applications in {t4}. The theoretical foundations of {t1} have been extensively analyzed by scholars worldwide. Many breakthroughs in {t2} can be traced back to early work on {t1}.",
    "Recent advances in {t1} have transformed our understanding of {t2}. By integrating techniques from {t3} with insights from {t4}, researchers have achieved remarkable progress. The implications of this work extend beyond {domain} into many practical applications.",
    "A comprehensive survey of {t1} reveals deep connections to {t2} that were previously unknown. Practitioners working with {t3} have found that {t4} provides complementary perspectives. This interdisciplinary approach has led to significant advances in both theory and practice.",
    "The relationship between {t1} and {t2} has been a subject of intense study. Early investigations focused on {t3}, while modern approaches incorporate {t4} for more nuanced analysis. This evolution in methodology has opened new avenues for research and application.",
    "Understanding {t1} requires familiarity with {t2} and its underlying principles. The integration of {t3} into existing frameworks has enabled researchers to tackle problems involving {t4} more effectively. These developments represent a paradigm shift in how we approach complex challenges.",
    "Practical applications of {t1} span multiple areas including {t2} and {t3}. The synergy between these concepts and {t4} has led to innovative solutions that were previously thought impossible. Current research aims to further optimize these approaches for real-world deployment.",
]

extra_sentences = [
    "Further research has expanded our understanding significantly.",
    "The connection between these concepts is well established in the literature.",
    "Applications continue to grow in both theoretical and practical domains.",
    "Several key publications have addressed the relationship between these concepts.",
    "This topic has attracted significant attention from the research community.",
    "New experimental results have confirmed many theoretical predictions.",
    "The practical implications of this work are far reaching.",
    "Cross disciplinary collaboration has accelerated progress in this area.",
    "Computational advances have enabled more sophisticated analysis.",
    "Historical context provides important perspective on recent developments.",
    "Teaching and education in this area have also evolved significantly.",
    "International conferences regularly feature sessions dedicated to this topic.",
    "Funding agencies have recognized the importance of continued investment.",
    "Open source tools have democratized access to cutting edge methods.",
    "The ethical implications of these advances deserve careful consideration.",
]

def generate_document(doc_id):
    domain_name = random.choice(list(topics.keys()))
    data = topics[domain_name]
    terms = random.sample(data["terms"], min(4, len(data["terms"])))
    template = random.choice(templates)
    readable_domain = domain_name.replace("_", " ")

    title = f"{terms[0].title()} and {terms[1].title()}"
    text = template.format(domain=readable_domain, t1=terms[0], t2=terms[1], t3=terms[2], t4=terms[3])

    # add 2-4 extra sentences for length variety
    text += " " + " ".join(random.sample(extra_sentences, random.randint(2, 4)))

    # 30% chance to add a cross-domain sentence
    if random.random() < 0.3:
        other_domain = random.choice(list(topics.keys()))
        other_terms = random.sample(topics[other_domain]["terms"], 2)
        text += f" This work also connects to {other_terms[0]} and {other_terms[1]} in {other_domain.replace('_', ' ')}."

    return {"id": str(doc_id), "title": title, "text": text}

def main():
    output_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(output_dir, "sample_corpus.jsonl")
    num_docs = 50000

    total_terms = sum(len(d["terms"]) for d in topics.values())
    print(f"generating {num_docs} documents across {len(topics)} domains ({total_terms} unique terms)...")

    with open(output_path, "w") as f:
        for i in range(num_docs):
            doc = generate_document(i)
            f.write(json.dumps(doc) + "\n")
            if (i + 1) % 10000 == 0:
                print(f"  generated {i + 1} documents")

    file_size = os.path.getsize(output_path)
    print(f"wrote {num_docs} documents to {output_path}")
    print(f"file size: {file_size / (1024 * 1024):.1f} MB")

if __name__ == "__main__":
    main()
