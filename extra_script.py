Import("env")

env.Replace(PROGNAME="Gluco-Family_%s" % env.GetProjectOption("custom_prog_version"))