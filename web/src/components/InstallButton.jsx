import { AlertIcon, BrowserIcon } from './icons.jsx'

// Wraps the <esp-web-install-button> web component (registered in main.jsx via
// `import 'esp-web-tools'`). The slots let us fully control the French copy for
// each state: activate (ready), unsupported (wrong browser), not-allowed (no HTTPS).
export default function InstallButton() {
  return (
    <esp-web-install-button manifest="./manifest.json">
      {/* Shown when the browser supports Web Serial */}
      <button
        slot="activate"
        className="group inline-flex items-center gap-3 rounded-2xl bg-brand-600 px-8 py-5 text-lg font-semibold text-white shadow-lg shadow-brand-600/30 transition hover:bg-brand-700 hover:shadow-xl active:scale-[0.98]"
      >
        <span className="grid h-7 w-7 place-items-center rounded-full bg-white/20 text-xl leading-none">⚡</span>
        Flasher ma carte
      </button>

      {/* Shown on Firefox / Safari / mobile (no Web Serial) */}
      <div
        slot="unsupported"
        className="mx-auto max-w-md rounded-2xl border border-amber-300 bg-amber-50 p-5 text-left"
      >
        <div className="flex items-center gap-2 font-semibold text-amber-800">
          <BrowserIcon className="h-5 w-5" />
          Ce navigateur ne peut pas flasher
        </div>
        <p className="mt-2 text-sm text-amber-800">
          Le flash par navigateur fonctionne uniquement sur <strong>ordinateur</strong>, avec un
          navigateur basé sur Chromium : <strong>Chrome, Edge, Brave</strong> ou Opera. Il ne marche
          pas sur Firefox, Safari, ni sur téléphone.
        </p>
        <p className="mt-2 text-sm text-amber-800">
          👉 Ouvrez cette page dans Chrome ou Edge sur un ordinateur, puis revenez ici.
        </p>
      </div>

      {/* Shown if the page is not served over HTTPS */}
      <div
        slot="not-allowed"
        className="mx-auto max-w-md rounded-2xl border border-red-300 bg-red-50 p-5 text-left"
      >
        <div className="flex items-center gap-2 font-semibold text-red-800">
          <AlertIcon className="h-5 w-5" />
          Accès au port USB bloqué
        </div>
        <p className="mt-2 text-sm text-red-800">
          La page doit être ouverte en <strong>https://</strong> pour accéder au port USB.
          Rechargez la page en https, ou réessayez avec Chrome / Edge.
        </p>
      </div>
    </esp-web-install-button>
  )
}
