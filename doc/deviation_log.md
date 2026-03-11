# DSP Embedded – Deviation Log

All deviations from the [Dataspace Protocol specification](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol) are listed here with rationale and interoperability impact.

| ID | Endpoint / Feature | Deviation | Rationale | Interoperability Impact |
|----|-------------------|-----------|-----------|------------------------|
| DEV-001 | JSON-LD processing | JSON-LD contexts are statically pre-compiled at build time; no runtime JSON-LD processor is present. | An RDF/JSON-LD runtime would exceed the available RAM budget on the ESP32-S3. | Counterparts must use only the standard DSP namespaces. Custom JSON-LD extensions are not supported. |
| DEV-002 | DAPS / OAuth2 | Full DAPS/OAuth2 is not implemented on the node. Identity uses pre-provisioned X.509 certificates with local JWT validation. | DAPS requires a complex OAuth2 PKI stack that is infeasible on a microcontroller. | Counterparts must accept X.509-based identity or connect via the optional DAPS gateway shim (DSP-205). |
| DEV-003 | Consumer role | The node acts as data provider only; consumer-side endpoints are not implemented. | Reduces code size and complexity for the primary embedded sensor use case. | The node cannot initiate data requests; it only responds to them. |
