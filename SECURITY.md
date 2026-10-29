# Local access and security

MedBox is a local bench prototype. Its default Wi-Fi password, **medbox-setup**, is intentionally documented and is not a private production credential. HTTP is unencrypted and anyone who can reach the local server can read its status and per-boot form token. The token/stale revision checks help prevent accidental cross-origin writes and conflicting edits; they are not user accounts or strong authorization.

Do not expose this HTTP server through port forwarding, a public tunnel or an untrusted shared network. Add suitable authentication, transport protection and deployment-specific controls before changing that scope. No such internet exposure is configured by this repository.

Real existing-network credentials belong only in the ignored `firmware/esp32/secrets.h`. Start with `secrets.example.h`; never put actual passwords/tokens in source, screenshots, logs or issue reports. Build outputs, flash/RAM dumps, local auth configuration and serial logs stay outside the public repository through `.gitignore`.

The USB maintenance interface can submit the same configuration requests as the dashboard. Treat physical USB access as control access. Test utilities can alter real reminders; restore temporary configurations and verify the final saved state.

An FC-51 lid opening is not evidence of swallowing medicine. Clock warnings and storage/save failures must stay visible. The tested assembly's depleted RTC backup cell remains a documented hardware issue.

For vulnerability reports, use a private reporting route if the repository owner enables one. Avoid including credentials or identifying medical information in public issues. For ordinary reproducible bugs, use the issue template with sanitized logs and board/module details.
