# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib.Configure import conf

OVERRIDE = '''
class Base
{
  virtual void
  f(int a);
};

class Derived : public Base
{
  virtual void
  f(int a) override;
};

class Final : public Derived
{
  virtual void
  f(int a) final;
};
'''

@conf
def check_override(self):
    if self.check_cxx(msg='Checking for override and final specifiers',
                      fragment=OVERRIDE,
                      features='cxx', mandatory=False):
        self.define('HAVE_CXX_OVERRIDE_FINAL', 1)

def configure(conf):
    conf.check_override()
