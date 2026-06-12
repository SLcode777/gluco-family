// Simple stroke icons (no external icon library) — sized via the `className` prop.
const base = {
  fill: 'none',
  stroke: 'currentColor',
  strokeWidth: 1.8,
  strokeLinecap: 'round',
  strokeLinejoin: 'round',
  viewBox: '0 0 24 24',
}

export function UsbIcon({ className }) {
  // Standard USB "trident" symbol.
  return (
    <svg {...base} className={className}>
      <circle cx="10" cy="7" r="1" />
      <circle cx="4" cy="20" r="1" />
      <path d="M4.7 19.3 19 5" />
      <path d="m21 3-3 1 2 2z" />
      <path d="M9.26 7.68 5 12l2 5" />
      <path d="m10 14 5 2 3.5-3.5" />
      <path d="m18 12 1-1 1 1-1 1z" />
    </svg>
  );
}

export function ChipIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <rect x="7" y="7" width="10" height="10" rx="1.5" />
      <path d="M10 2v3M14 2v3M10 19v3M14 19v3M2 10h3M2 14h3M19 10h3M19 14h3" />
    </svg>
  )
}

export function LanguageIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <circle cx="12" cy="12" r="9" />
      <path d="M3 12h18M12 3c2.5 2.5 2.5 15 0 18M12 3c-2.5 2.5-2.5 15 0 18" />
    </svg>
  )
}

export function ClockIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <circle cx="12" cy="12" r="9" />
      <path d="M12 7v5l3 2" />
    </svg>
  )
}

export function WifiIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M2 8.5a16 16 0 0 1 20 0M5 12a11 11 0 0 1 14 0M8 15.5a6 6 0 0 1 8 0" />
      <circle cx="12" cy="19" r="1.2" />
    </svg>
  )
}

export function UserIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <circle cx="12" cy="8" r="4" />
      <path d="M4 21a8 8 0 0 1 16 0" />
    </svg>
  )
}

export function UsersIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <circle cx="9" cy="8" r="3.5" />
      <path d="M3 20a6 6 0 0 1 12 0" />
      <path d="M16 5.5a3.5 3.5 0 0 1 0 7M21 20a6 6 0 0 0-5-5.9" />
    </svg>
  )
}

export function BrowserIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <rect x="3" y="4" width="18" height="16" rx="2" />
      <path d="M3 9h18" />
      <circle cx="6" cy="6.5" r="0.6" />
      <circle cx="8.5" cy="6.5" r="0.6" />
    </svg>
  )
}

export function LaptopIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <rect x="5" y="5" width="14" height="9" rx="1.5" />
      <path d="M2 18h20l-1.5-2.5h-17L2 18z" />
    </svg>
  )
}

export function CheckIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M4 12l5 5L20 6" />
    </svg>
  )
}

export function AlertIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M12 3l9 16H3l9-16z" />
      <path d="M12 9v4M12 16.5v.2" />
    </svg>
  )
}

export function DownloadIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M12 3v12M7 10l5 5 5-5" />
      <path d="M4 20h16" />
    </svg>
  )
}

export function CopyIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <rect x="9" y="9" width="11" height="11" rx="2" />
      <path d="M5 15V5a2 2 0 0 1 2-2h8" />
    </svg>
  );
}

export function CursorIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M5 3l5 17 2.4-6.6L19 11 5 3z" />
    </svg>
  );
}

export function RefreshIcon({ className }) {
  return (
    <svg {...base} className={className}>
      <path d="M20 11a8 8 0 0 0-14-4.5L4 8" />
      <path d="M4 4v4h4" />
      <path d="M4 13a8 8 0 0 0 14 4.5L20 16" />
      <path d="M20 20v-4h-4" />
    </svg>
  )
}
