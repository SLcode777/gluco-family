import InstallButton from './components/InstallButton.jsx'
import UpdateSection from './components/UpdateSection.jsx'
import PhotoSlot from './components/PhotoSlot.jsx'
import {
  UsbIcon,
  ChipIcon,
  LanguageIcon,
  ClockIcon,
  WifiIcon,
  UserIcon,
  UsersIcon,
  BrowserIcon,
  LaptopIcon,
  CheckIcon,
} from './components/icons.jsx'

// Steps shown on the device's own touch screen, after the flash.
const deviceSteps = [
  {
    icon: LanguageIcon,
    title: 'Choisir la langue',
    text: "À l'allumage, l'écran propose plusieurs langues. Touchez « Français ».",
    photo: "L'écran d'accueil avec le choix de la langue",
  },
  {
    icon: ClockIcon,
    title: 'Choisir le fuseau horaire',
    text: 'Sélectionnez votre fuseau (ex. Paris) pour afficher la bonne heure.',
    photo: 'La liste des fuseaux horaires',
  },
  {
    icon: WifiIcon,
    title: 'Se connecter au WiFi',
    text: 'Touchez votre réseau WiFi dans la liste, puis tapez le mot de passe sur le clavier à l’écran.',
    photo: 'La sélection du réseau WiFi',
  },
  {
    icon: UserIcon,
    title: 'Entrer le compte Dexcom',
    text: 'Saisissez l’identifiant et le mot de passe Dexcom du premier enfant. C’est ce compte qui fournit les glycémies.',
    photo: 'L’écran de saisie du compte Dexcom',
  },
]

function SectionTitle({ kicker, children }) {
  return (
    <div className="text-center">
      {kicker && (
        <div className="mb-2 text-sm font-semibold uppercase tracking-wide text-brand-600">
          {kicker}
        </div>
      )}
      <h2 className="text-2xl font-bold text-slate-900 sm:text-3xl">{children}</h2>
    </div>
  )
}

export default function App() {
  return (
    <div className="min-h-screen">
      {/* ---------- HERO ---------- */}
      <header className="mx-auto max-w-3xl px-5 pt-16 pb-10 text-center sm:pt-24">
        <div className="mb-5 inline-flex items-center gap-2 rounded-full bg-brand-100 px-4 py-1.5 text-sm font-medium text-brand-700">
          <ChipIcon className="h-4 w-4" />
          Installation en 1 clic
        </div>
        <h1 className="text-4xl font-extrabold tracking-tight text-slate-900 sm:text-5xl">
          Installez <span className="text-brand-600">Gluco-Family</span>
          <br />
          sur votre carte
        </h1>
        <p className="mx-auto mt-5 max-w-xl text-lg text-slate-600">
          Branchez votre carte, cliquez sur un bouton, et c’est installé. Pas de logiciel à
          télécharger, pas de ligne de commande. On vous guide ensuite pas à pas.
        </p>
        <a
          href="#installer"
          className="mt-8 inline-flex items-center gap-2 rounded-2xl bg-brand-600 px-7 py-4 text-base font-semibold text-white shadow-lg shadow-brand-600/30 transition hover:bg-brand-700"
        >
          Commencer
          <span aria-hidden>↓</span>
        </a>
      </header>

      {/* ---------- PREREQUIS ---------- */}
      <section className="mx-auto max-w-3xl px-5 py-6">
        <div className="rounded-3xl border border-slate-200 bg-white p-6 shadow-sm sm:p-8">
          <h2 className="text-center text-lg font-semibold text-slate-900">
            Avant de commencer, il vous faut :
          </h2>
          <div className="mt-6 grid gap-4 sm:grid-cols-3">
            <Prereq icon={LaptopIcon} title="Un ordinateur">
              Windows, Mac ou Linux. <strong>Pas un téléphone</strong> ni une tablette.
            </Prereq>
            <Prereq icon={BrowserIcon} title="Chrome, Edge ou Brave">
              Un navigateur basé sur Chromium. Pas Firefox ni Safari.
            </Prereq>
            <Prereq icon={UsbIcon} title="Un câble USB">
              Pour relier la carte à l’ordinateur. Un câble qui transfère les données (pas
              seulement la charge).
            </Prereq>
          </div>
        </div>
      </section>

      {/* ---------- ETAPE 1 : BRANCHER + FLASHER ---------- */}
      <section id="installer" className="mx-auto max-w-4xl px-5 py-14 scroll-mt-6">
        <SectionTitle kicker="Étape 1">Brancher et flasher la carte</SectionTitle>

        <div className="mt-10 grid items-center gap-8 sm:grid-cols-2">
          <div className="order-2 sm:order-1">
            <ol className="space-y-4">
              <Bullet n="1">
                Reliez la carte à l’ordinateur avec le câble USB.
              </Bullet>
              <Bullet n="2">
                Cliquez sur <strong>« Flasher ma carte »</strong> ci-dessous.
              </Bullet>
              <Bullet n="3">
                Une petite fenêtre s’ouvre : choisissez le port qui apparaît (souvent nommé
                « USB Serial » ou « ESP32 »), puis <strong>Connexion</strong>.
              </Bullet>
              <Bullet n="4">
                Laissez faire jusqu’à <strong>100 %</strong>. Ne débranchez pas la carte pendant
                l’installation.
              </Bullet>
            </ol>
          </div>
          <div className="order-1 sm:order-2">
            <PhotoSlot caption="La carte branchée en USB à l’ordinateur" />
          </div>
        </div>

        {/* Le bouton */}
        <div className="mt-12 flex flex-col items-center gap-4 rounded-3xl border border-brand-200 bg-brand-50/60 p-8 text-center">
          <InstallButton />
          <p className="max-w-md text-sm text-slate-500">
            Rien ne se passe au clic ? Vérifiez que la carte est bien branchée et que vous êtes
            sur Chrome, Edge ou Brave, sur un ordinateur.
          </p>
        </div>
      </section>

      {/* ---------- ETAPE 2 : CONFIG SUR L'ECRAN ---------- */}
      <section className="bg-white py-14">
        <div className="mx-auto max-w-4xl px-5">
          <SectionTitle kicker="Étape 2">La configuration, sur l’écran de la carte</SectionTitle>
          <p className="mx-auto mt-4 max-w-xl text-center text-slate-600">
            Une fois flashée, la carte s’allume et affiche son écran de bienvenue. Tout se fait
            ensuite <strong>au doigt, sur l’écran tactile</strong>.
          </p>

          <div className="mt-10 grid gap-6 sm:grid-cols-2">
            {deviceSteps.map((step, i) => (
              <DeviceStep key={step.title} n={i + 1} {...step} />
            ))}
          </div>
        </div>
      </section>

      {/* ---------- ETAPE 3 : LES AUTRES MEMBRES ---------- */}
      <section className="mx-auto max-w-4xl px-5 py-14">
        <SectionTitle kicker="Étape 3">Ajouter les autres membres de la famille</SectionTitle>
        <div className="mt-10 grid items-center gap-8 sm:grid-cols-2">
          <div>
            <div className="mb-3 inline-flex items-center gap-2 rounded-full bg-brand-100 px-3 py-1 text-sm font-medium text-brand-700">
              <UsersIcon className="h-4 w-4" />
              Jusqu’à 3 personnes
            </div>
            <ol className="space-y-4">
              <Bullet n="1">
                Sur l’écran de la carte, repérez l’<strong>adresse</strong> affichée (du type
                <code className="mx-1 rounded bg-slate-100 px-1.5 py-0.5 text-sm">192.168.x.x</code>).
              </Bullet>
              <Bullet n="2">
                Depuis votre téléphone ou ordinateur <em>connecté au même WiFi</em>, ouvrez cette
                adresse dans le navigateur, puis allez sur la page <strong>Réglages</strong>.
              </Bullet>
              <Bullet n="3">
                Ajoutez le nom et le compte Dexcom de chaque enfant, puis enregistrez. Les
                glycémies des 3 membres s’affichent alors côte à côte sur la carte. 🎉
              </Bullet>
            </ol>
          </div>
          <PhotoSlot caption="La page Réglages dans le navigateur" />
        </div>
      </section>

      {/* ---------- MISE A JOUR (OTA) ---------- */}
      <UpdateSection />

      {/* ---------- FAQ ---------- */}
      <section className="bg-white py-14">
        <div className="mx-auto max-w-2xl px-5">
          <SectionTitle>Petits soucis fréquents</SectionTitle>
          <div className="mt-8 space-y-3">
            <Faq q="Le bouton « Flasher » est grisé ou rien ne se passe.">
              Vous êtes sûrement sur Firefox, Safari, ou sur un téléphone. Ouvrez cette page sur un
              ordinateur avec Chrome, Edge ou Brave. Sur Brave, si ça bloque encore, baissez le
              bouclier (icône Brave dans la barre d’adresse).
            </Faq>
            <Faq q="Aucun port n’apparaît dans la fenêtre de connexion.">
              Le câble USB est peut-être un câble « charge seule ». Essayez un autre câble, et un
              autre port USB de l’ordinateur. Débranchez puis rebranchez la carte.
            </Faq>
            <Faq q="L’écran de la carte reste noir après le flash.">
              Débranchez puis rebranchez la carte pour la redémarrer. Le premier démarrage peut
              prendre quelques secondes.
            </Faq>
            <Faq q="Je veux juste mettre à jour une carte déjà installée.">
              Le bouton « Flasher ma carte » sert à la <strong>première installation</strong> (il
              remet la carte à neuf). Pour une simple mise à jour qui conserve votre configuration,
              utilisez la section <strong>« Mettre à jour la carte »</strong> ci-dessus.
            </Faq>
          </div>
        </div>
      </section>

      {/* ---------- FOOTER ---------- */}
      <footer className="border-t border-slate-200 py-10 text-center text-sm text-slate-500">
        <p>
          Gluco-Family — fait avec ❤️ pour les familles concernées par le diabète de type 1.
        </p>
        <p className="mt-1">
          Outil maison, sans lien avec Dexcom. Ne remplace pas un dispositif médical.
        </p>
      </footer>
    </div>
  )
}

function Prereq({ icon: Icon, title, children }) {
  return (
    <div className="rounded-2xl bg-slate-50 p-4 text-center">
      <div className="mx-auto grid h-12 w-12 place-items-center rounded-full bg-brand-100 text-brand-600">
        <Icon className="h-6 w-6" />
      </div>
      <div className="mt-3 font-semibold text-slate-900">{title}</div>
      <p className="mt-1 text-sm text-slate-600">{children}</p>
    </div>
  )
}

function Bullet({ n, children }) {
  return (
    <li className="flex gap-3">
      <span className="grid h-7 w-7 flex-none place-items-center rounded-full bg-brand-600 text-sm font-bold text-white">
        {n}
      </span>
      <span className="pt-0.5 text-slate-700">{children}</span>
    </li>
  )
}

function DeviceStep({ n, icon: Icon, title, text, photo }) {
  return (
    <div className="rounded-2xl border border-slate-200 bg-white p-5">
      <div className="flex items-center gap-3">
        <span className="grid h-9 w-9 flex-none place-items-center rounded-full bg-brand-600 text-sm font-bold text-white">
          {n}
        </span>
        <Icon className="h-6 w-6 text-brand-600" />
        <h3 className="font-semibold text-slate-900">{title}</h3>
      </div>
      <p className="mt-3 text-sm text-slate-600">{text}</p>
      <div className="mt-4">
        <PhotoSlot ratio="aspect-[3/2]" caption={photo} />
      </div>
    </div>
  )
}

function Faq({ q, children }) {
  return (
    <details className="group rounded-2xl border border-slate-200 bg-slate-50 p-4 open:bg-white">
      <summary className="flex cursor-pointer items-center justify-between gap-3 font-medium text-slate-900 marker:content-['']">
        <span className="flex items-center gap-2">
          <CheckIcon className="h-5 w-5 flex-none text-brand-500" />
          {q}
        </span>
        <span className="text-slate-400 transition group-open:rotate-180" aria-hidden>
          ⌄
        </span>
      </summary>
      <p className="mt-3 pl-7 text-sm text-slate-600">{children}</p>
    </details>
  )
}
