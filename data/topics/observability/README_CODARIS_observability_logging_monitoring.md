# CODARIS #14: Observability, Logging & Monitoring

Generated: 23 September 2026

## Scope

This pack contains **172 catalogue entries** for **Observability, Logging & Monitoring**.

Separates metrics, telemetry collection, logs, traces, APM platforms, errors, uptime, incident response, profiling and eBPF/runtime observability. OpenTelemetry is instrumentation/collection, not itself a backend database.

## Research integrity

- Popularity/adoption values are kept tied to their named source, year and denominator/scope.
- Survey percentages are not treated as global market share unless the source explicitly measures market share.
- Country-by-tool and gender-by-tool percentages are not fabricated when no defensible cross-tab exists.
- Overlap with other CODARIS categories is allowed when a technology genuinely spans layers, but its role is labelled here.

## Files

- `CODARIS_observability_logging_monitoring_research.xlsx`
- `codaris_observability_logging_monitoring_catalog.csv`
- `codaris_observability_logging_monitoring_catalog.json`
- `codaris_observability_logging_monitoring_sources.csv`

## Recommended CODARIS fields

Name, class, subtype, ecosystem/vendor, primary use, deployment/interface, source-specific adoption metric (where available), notes and source provenance.

## Source-specific adoption signals

- **Prometheus**: 77.0% — CNCF 2025 graduated-project production use. Scope: CNCF annual survey.
- **Fluentd**: 41.0% — CNCF 2025 graduated-project production use. Scope: CNCF annual survey.
- **Jaeger**: 22.0% — CNCF 2025 graduated-project production use. Scope: CNCF annual survey.
- **Prometheus exporters**: 72.0% — Prometheus/OTel 2026 infrastructure instrumentation. Scope: 81 screened active OTel+Prometheus users.
- **OpenTelemetry receivers**: 57.0% — Prometheus/OTel 2026 infrastructure instrumentation. Scope: 81 screened active OTel+Prometheus users.
- **OpenTelemetry SDKs**: 65.0% — Prometheus/OTel 2026 application instrumentation. Scope: 81 screened active OTel+Prometheus users.

## Key sources

- **CNCF 2025 Annual Survey** — https://www.cncf.io/announcements/2026/01/20/kubernetes-established-as-the-de-facto-operating-system-for-ai-as-production-use-hits-82-in-2025-cncf-annual-cloud-native-survey/ — Prometheus 77% production; Fluentd 41%; Jaeger 22% among graduated-project respondents.
- **CNCF 2025 survey announcement** — https://www.cncf.io/announcements/2026/01/20/kubernetes-established-as-the-de-facto-operating-system-for-ai-as-production-use-hits-82-in-2025-cncf-annual-cloud-native-survey/ — OpenTelemetry was the second-highest-velocity CNCF project.
- **Prometheus survey 2026** — https://prometheus.io/blog/2026/09/21/otel-prometheus-interoperability-survey/ — Among screened active users, Prometheus exporters 72%, OTel receivers 57%; OTel SDKs 65% for application instrumentation.
