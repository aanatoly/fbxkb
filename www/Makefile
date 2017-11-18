AUXSRC = top bottom1 bottom2
SRC = index.thtml faq.thtml

HTML = $(SRC:%.thtml=%.html)


all: $(HTML)
$(HTML) : $(AUXSRC) Makefile



%.html: %.thtml
	cat top $< bottom1  > $@
	echo "$(shell date '+Updated on %d-%m-%Y')" >> $@
	cat bottom2 >> $@
	@echo


clean:
	$(RM) $(HTML)
