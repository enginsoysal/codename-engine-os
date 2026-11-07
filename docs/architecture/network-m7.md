# M7 native networking

Status: the native Ethernet, TCP/TLS/HTTP and streaming download path is implemented. Final exact-artifact M7 qualification is in progress; historical implementation slices below remain identified.

## Driver and scheduling

An original Intel 8254x/82574 driver recognizes the 82540, 82545 and 82574 device IDs. QEMU exercises 82540 legacy IRQ and 82574 MSI. VMware uses 82574/e1000e MSI; its legacy e1000 interrupt route was unsupported and is explicitly excluded. Recognition is not universal qualification. Bounded descriptor rings use managed DMA pages and uncached supervisor MMIO. Interrupts acknowledge and flag work; bounded deferred batches parse packets in the desktop event loop. MSI is preferred, with checked legacy IRQ fallback. A quarantined device retains pages it might still DMA into rather than freeing unsafe memory.

DHCP, ARP and DNS use original bounded packet builders/parsers and software checksums. IPv4 fragments, malformed lengths/checksums and invalid DNS compression are refused. Configuration and DNS retries have deadlines. The PIT is enabled only while a request needs a deadline; completed requests return to interrupt-driven halt. Link loss clears configuration without changing documents, and link recovery restarts DHCP. One interface, IPv4, bounded caches and no hotplug are explicit limits.

## Entropy and security boundary

DNS transaction IDs and source ports require fresh RDRAND output with bounded retry. There is no TSC fallback for security-sensitive randomization. DHCP's non-cryptographic bootstrap identifier may use TSC if hardware entropy is absent. The initial default-QEMU test obtained DHCP but refused DNS because that CPU did not expose RDRAND. The positive test explicitly selects `max,x2apic=off`. Broader entropy sources must be implemented and qualified before promising support on other CPUs.

BearSSL 0.6 retains its upstream MIT notices and exact provenance under `third_party/bearssl`. All 277 portable sources compile freestanding with OS randomness/time and SIMD backends disabled. Actual TLS uses explicit RDRAND entropy, stable validated UTC, pinned public roots and hostname/chain/expiry checks. Independent cryptographic adapter cases and native positive/negative tasks qualify different parts of this path. No insecure certificate bypass or HTTP downgrade is permitted.

## Evidence and sources

The [native network test](../../evidence/M7/network/results.json) checks actual installed guest traffic, DHCP message types, ARP/DNS packets, cable recovery, halted CPUs and stopped painting. The [wire test](../../evidence/M7/wire-results.json) executes actual original C against independent fixtures and 30,000 deterministic mutations. Neither closes M7.

Protocol/register references: [Intel PCI/PCI-X controller software manual](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf), [IPv4 RFC 791](https://www.rfc-editor.org/rfc/rfc791.html), [DHCP RFC 2131](https://www.rfc-editor.org/rfc/rfc2131.html), [DNS RFC 1035](https://www.rfc-editor.org/rfc/rfc1035.html), [TCP RFC 9293](https://www.rfc-editor.org/rfc/rfc9293.html), and [BearSSL](https://www.bearssl.org/).

## Secure download development implementation

The original TCP client permits one active stream, one outstanding segment, a 16-KiB receive ring, bounded retries and a no-progress deadline. Device backpressure does not consume the caller's bytes. Sequence arithmetic handles wraparound, duplicate/out-of-order segments are acknowledged without duplicating data, and the receive window closes/reopens with buffer availability. This deliberately modest transport trades throughput for predictable memory and a smaller initial state machine. It does not yet provide a general socket service, IPv6, fragmentation, SACK, PMTU discovery or a production TCP conformance claim. [Actual C state-machine checks](../../evidence/M7/tcp-results.json) are host tests, not native NIC transport proof.

BearSSL's nonblocking engine is restricted to TLS 1.2, ECDHE with AES-GCM or ChaCha20-Poly1305, no renegotiation, fresh explicit entropy, explicit UTC validation time and the pinned public Mozilla/curl roots. No test root enters the production bundle. The root source and generated keys are digest-bound build inputs. Mozilla's MPL-2.0 CA data and its notices are retained separately from MIT code. The PEM bundle does not carry Firefox's external name constraints, revocation or CT policy; full browser-policy equivalence is not claimed. TLS 1.3 is not supported in this library version.

[Eight real cryptographic policy cases](../../evidence/M7/tls-policy-results.json) use an independent Python/OpenSSL memory peer and disposable test-only trust: valid transfer, hostname mismatch, untrusted certificate, expiry, future validation time, missing clock, missing entropy and transport truncation. These prove the actual wrapper/library decisions under that adapter, not native TCP/RTC operation. Earlier compile/link integration errors (missing freestanding string include, incorrect upstream flags API, adapter prototype, Windows command length/response quoting) were corrected before runtime testing; no failed native security run is represented as successful.

The bounded PC CMOS provider requires a stable valid RTC snapshot, handles BCD/binary and 12/24-hour format, and interprets the qualified virtual RTC as UTC. It fails closed outside 2020–2099; it is not authenticated network time. [29,220 calendar dates](../../evidence/M7/clock-results.json) match an independent calendar. Native negative tasks also set an invalid virtual RTC and verify refusal without changing the existing file.

The original [HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112.html) parser handles bounded headers, fixed/chunked bodies, checked redirect metadata and TLS-authenticated close-delimited completion. Ambiguous length/transfer framing, malformed/truncated input, unsupported content encoding and over-limit bodies are refused. [Fixtures and 5,000 mutations](../../evidence/M7/http-results.json) execute the actual parser. The download broker allows at most four HTTPS redirects, no downgrade, no credential/cookie forwarding, and a 64-MiB body bound.

The volume bridge stages at most 4 KiB and reserves an unreferenced contiguous extent while other documents can still allocate elsewhere. Cancellation leaves no partial namespace entry. Completion flushes the payload and commits current metadata; path conflicts refuse replacement. [Concurrent writes and 75 write/flush interruptions](../../evidence/M7/stream-results.json) preserve other documents and complete namespace entries. This is bounded COW behavior, not a universal power-loss guarantee. Native positive/negative downloads and resource accounting are separately bound by their reports and final acceptance.

## First native HTTPS task

The installed QEMU guest now completes actual DHCP, DNS, TCP and certificate/UTC-checked TLS 1.2, downloads the public Engine OS MIT license from `raw.githubusercontent.com`, and commits its exact 1,069 bytes to the native volume. An independent reader checks GPT, metadata/payload CRC and byte equality with the public license; a detached restart retains the file. The completed session stops repainting and both CPUs halt. [Exact positive-task evidence](../../evidence/M7/https/results.json) binds the build and ISO digest. This first positive task is not the negative-security, larger-file, VMware, resource-pressure or full M7 release matrix.

## Native negative cases and VMware routing

The [native negative suite](../../evidence/M7/https-negative/results.json) checks expired, hostname-mismatched and untrusted certificates, HTTP downgrade, cancellation, cable loss/recovery and invalid UTC while preserving existing files. Internet transport timeouts are recorded separately from certificate decisions; a bounded retry never substitutes a timeout for the required certificate error. Earlier input/interruption harness failures remain under `evidence/M7/history`.

The first owned VMware EFI test quarantined an unsupported legacy e1000 IRQ route. The qualified route is e1000e MSI. A missing PCIe root port initially prevented VMware itself from starting; that configuration failure is retained and distinguished from a guest crash. The owned test configures a PCIe root port, verifies its loopback-only RFB listener, captures the real guest framebuffer, types into the actual Editor and saves a document. It never automates the host desktop. Both BIOS and UEFI desktop/download/restart runs are separate gates, with the existing VM inventory unchanged.

## Larger signed redirects

GitHub's public release download redirects to a longer signed HTTPS URL. The checked URL parser permits a 253-byte hostname, a path of at most 2,047 bytes and a total URL shorter than 3,072 bytes. HTTP line and aggregate header/trailer limits remain bounded; redirect depth stays four. The native test downloads the original public 7-Zip 26.05 x64 installer and independently checks its 1,663,506 bytes, MZ signature and SHA-256 after restart. This proves downloading bytes, not running a Windows program. The download does not grow managed allocations with file size. General browser behavior remains out of scope.
