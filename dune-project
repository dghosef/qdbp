(lang dune 3.4)

(name qdbp)

(generate_opam_files true)

(source
 (github username/reponame))

(authors "Author Name")

(maintainers "Maintainer Name")

(license LICENSE)

(documentation https://url/to/documentation)

(package
 (name qdbp)
 (synopsis "A short synopsis")
 (description "A longer description")
 (depends ocaml dune ocamllex)
 (tags
  (topics "to describe" your project)))
(using menhir 2.0)

; See the complete stanza docs at https://dune.readthedocs.io/en/stable/dune-files.html#dune-project
