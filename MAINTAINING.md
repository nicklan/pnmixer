Maintaining
===========

Table of Contents
-----------------

* [Releasing](#releasing)
* [Translating](#translating)

Releasing
---------

When we enter RC stage...

- ensure that version in `CMakeLists.txt` is the right one
- translations should be updated (see below)
- RC releases can be created simply with git tags (preferably signed)

For final releases, we want to create an archive manually, additionaly to the
tag. This is to ensure that we have a persistent archive for a given version.
The procedure looks like that.

	export version=0.7.2
	export tag=v${version}
	git tag -s -m "Release ${tag}" ${tag}
	git archive --prefix=pnmixer-${version}/ --format=tar.gz -o pnmixer-${version}.tar.gz ${tag}
	sha256sum --tag pnmixer-${version}.tar.gz > SHA256
	gpg --armor --detach-sig SHA256
	gpg --armor --detach-sig pnmixer-${version}.tar.gz
	signify -S -s pnmixer_signify.sec  -e -m SHA256
	unset tag version

Then upload the files `SHA256`, `SHA256.asc`, `SHA256.sig`,
`pnmixer-${version}.tar.gz` and `pnmixer-${version}.tar.gz.asc` to the GitHub
release page.

Translating
-----------

When a new version of PNMixer is about to be released, we need to inform the
[Translation Project](https://translationproject.org) so that they can update
the translations. The TP just needs to know where to download an up to date
archive of PNMixer. This archive must contain an up to date POT file.

The TP is considered as our upstream when it comes to the PO files. Our job
is to provide them an updated version of the POT file before each new release,
and their job is to provide us with freshly translated PO files. There should
be no exception to this workflow. We should never modify the PO files ourselves,
and we should never accept translations from another channel than the TP.

In order to update the POT file, one has to run the following command from the
`build` directory.

	make -C po update-pot
	# Then commit the new POT file

The procedure for announcing a new version of PNMixer to the TP is thoroughly
described on the [maintainters](https://translationproject.org/html/maintainers.html)
page. In a nutshell, send a mail to <coordinator@translationproject.org>, use
`pnmixer-<version>.pot` as the subject, and include in the body the url of the
new PNMixer archive.

So, let's sum up the steps to follow to update the translations.

- update the pot file, commit.
- tag a new version (don't forget to bump the version where need be), commit.
- push the changes and the tags.
- have a look on the GitHub release page, copy the url of the `tar.gz` archive.
- send a mail to the TP with this url.

Please note that the TP will process a POT file only once, so another submission
must use a newer version.

To know about the current translation status, visit the [pnmixer textual domain
](https://translationproject.org/domain/pnmixer.html) page on the TP.
Whenever translations are updated, we (elboulangero) get notified by email.

The latest translations can be imported back in PNMixer at any time, using
the following command.

	rsync -Lrtvz --exclude=ru.po --exclude=zh_CN.po \
	    translationproject.org::tp/latest/pnmixer/ po/
	# Then commit the new PO files

As you can see, we exclude two translations. The reason is that I accepted these
translations as solo patches some time ago, and afterwards got in touch with the
TP about this matter, which resulted in having the two languages marked as external.
Nobody will update it on the TP side, and our local versions are newer, so we must
exclude it from the rsync.

We can live with that, but if the authors of these two translations don't show up
to provide up to date versions, we should get back in touch with the TP and try
to sort that out. Ultimately, we want no exception of this kind, only translations
from the TP.

Anyway.

After importing translations, don't forget to:

- update the translators list in the "About" dialog.
- update the `ChangeLog`.
- if new languages have been added, add them to the file `po/LINGUAS`.
