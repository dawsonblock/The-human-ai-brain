
AI Cockpit

A modular orchestration framework for AI models and memory-based reasoning.  This open-source project provides a secure, configurable API layer that can coordinate multiple large language models, recall context from vector memory, and manage an entropy-driven working memory.  It is designed to be production-ready while remaining simple to deploy for development and exploration.

Features
	•	Multi-model routing: Register multiple LLM backends and automatically select the appropriate model based on input length or cost/latency budgets (configurable via environment variables).
	•	Working memory gate: An entropy-based mechanism that determines when to reuse existing context or reset working memory for fresh reasoning.
	•	Vector memory recall: Integrate with a Qdrant or other vector database to retrieve relevant past conversations or documents, injecting them into the prompt as context.
	•	Extensible adapters: Example adapters for llama.cpp and OCR models are included.  Additional adapters can be written by implementing the simple interface defined in cockpit_mod/adapters/base.py.
	•	FastAPI service: Provides /chat and /selfwrite endpoints secured by API keys (JWT-ready).  Health endpoints (/live, /ready) allow Kubernetes or other orchestrators to manage lifecycle and readiness.
	•	Self-write pipeline (optional): A scaffold for controlled self-modification.  Patches are gated by a policy, tested in isolation, and attested before being merged.  Use with caution.
	•	Observability: Prometheus metrics and basic OpenTelemetry tracing are integrated.  A sample Grafana dashboard is provided for latency, working memory events, and recall statistics.

Getting Started

Requirements
	•	Python 3.10 or newer
	•	pip for installing dependencies
	•	(Optional) Docker and Docker Compose if you want to run the full stack locally

Installation

# Clone this repository
git clone https://github.com/yourorg/ai-cockpit.git
cd ai-cockpit

# Install Python dependencies
pip install -r cockpit_mod/requirements.txt

Running the API Server

# Export an API key for authentication (comma-separated list for multiple keys)
export COCKPIT_API_KEYS=devkey123

# Start the FastAPI server locally
python -m cockpit_mod.run_server

The server will listen on port 8000 by default.  Adjust HOST and PORT environment variables to bind to different addresses.

Docker Compose (local full stack)

For development, you can run the API alongside Qdrant, vLLM, Redis, Prometheus, and Grafana using Docker Compose:

cd infra
# Ensure you have set COCKPIT_API_KEYS (and optionally COCKPIT_JWT_SECRET_KEY) in your environment
docker compose up --build

Then you can interact with the API at http://localhost:8000 and monitor metrics at http://localhost:3000 (Grafana).  Default Grafana credentials are admin:admin.

Deployment

For production, review the Kubernetes manifests under infra/.  These include a Deployment for the API, services, HorizontalPodAutoscaler, and an ingress example.  A separate StatefulSet example for a high-availability Qdrant cluster is provided in infra/qdrant-ha.yaml.  Adjust resource requests, TLS configuration, and environment variables according to your environment.  A sample GitHub Actions workflow for canary deployments is provided in .github/workflows/canary.yml.

Configuration

Configuration is controlled via environment variables prefixed with COCKPIT_.  Key variables include:
	•	COCKPIT_API_KEYS: Comma-separated list of API keys for authentication.
	•	COCKPIT_MODELS_CONFIG: Path to the YAML file describing available models and adapters (default cockpit_mod/config/models.yaml).
	•	COCKPIT_MEMORY_DB: Path or DSN for the episodic memory database (e.g. a Qdrant endpoint).
	•	COCKPIT_WM_THRESHOLD: Entropy threshold for the working memory gate (default 1.39 bits).
	•	COCKPIT_WM_DECAY: Decay factor for the working memory (default 0.1).
	•	COCKPIT_RATE_LIMIT_REDIS_URL: Redis connection string for rate limiting (optional).
	•	COCKPIT_QDRANT_HOST, COCKPIT_QDRANT_PORT: Qdrant connection details.
	•	COCKPIT_ROUTER_GOVERNOR: Set to cost to enable cost/SLA-based model selection.

Refer to cockpit_mod/config/settings.py for the full set of supported settings.

Extending
	•	Adding a new model: Create a new adapter in cockpit_mod/adapters that subclasses ModelAdapter and implements chat() and embed().  Add it to config/models.yaml.
	•	Custom routing: Modify cockpit_mod/orchestrator/router_rules.py or router_governor.py to implement your own selection logic.
	•	Self-write policies: Edit the OPA policy in opa/selfwrite_gate.rego and the Python gate logic in cockpit_mod/selfwrite/gate.py to enforce your organization’s code change requirements.

Community

Contributions are welcome!  Please open issues or pull requests if you have questions or improvements.  Be mindful that this project emphasizes safety and clear boundaries when it comes to self-modifying code and LLM orchestration.