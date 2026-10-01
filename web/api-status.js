(() => {
  "use strict";

  const state = document.getElementById("health-state");
  const label = document.getElementById("health-label");
  const message = document.getElementById("health-message");
  const time = document.getElementById("health-time");
  const version = document.getElementById("api-version");
  const contracts = document.getElementById("api-contracts");
  const accessContract = document.getElementById("api-access-contract");
  const copyrightYear = document.querySelector("[data-copyright-year]");

  if (copyrightYear) {
    const currentYear = Math.max(2026, new Date().getFullYear());
    copyrightYear.textContent = currentYear > 2026 ? `2026–${currentYear}` : "2026";
  }

  async function refreshStatus() {
    const controller = new AbortController();
    const timeout = window.setTimeout(() => controller.abort(), 5000);
    try {
      const response = await fetch("/api/health", {
        method: "GET",
        headers: { Accept: "application/json" },
        cache: "no-store",
        credentials: "omit",
        signal: controller.signal,
      });
      if (!response.ok) throw new Error("health probe returned a non-success status");
      const health = await response.json();
      if (!health || health.message !== "Ready" || typeof health.version !== "string") {
        throw new Error("health response did not match the expected contract");
      }

      state.dataset.state = "online";
      label.textContent = "Operational";
      message.textContent = health.message;
      version.textContent = health.version;
      contracts.textContent = Number.isInteger(health.contact_api) && Number.isInteger(health.credential_api)
        ? `Contact v${health.contact_api} · Credential v${health.credential_api}`
        : "Contact · Credential";
      accessContract.textContent = Number.isInteger(health.page_access_api)
        ? `Page access v${health.page_access_api}`
        : "Page access available";
    } catch {
      state.dataset.state = "offline";
      label.textContent = "Status unavailable";
      message.textContent = "Health check could not be reached";
      version.textContent = "—";
      contracts.textContent = "—";
      accessContract.textContent = "Page access —";
    } finally {
      window.clearTimeout(timeout);
      const checked = new Intl.DateTimeFormat(undefined, { hour: "2-digit", minute: "2-digit", second: "2-digit" }).format(new Date());
      time.textContent = `GET /api/health · checked ${checked}`;
    }
  }

  void refreshStatus();
  window.setInterval(refreshStatus, 30000);
})();
