#!/usr/bin/env python

"""

Generate a new DATAMETRIC-derived class for VDrift's data management system.

Usage: python create_new_metric.py "Metric Name"
where "Metric Name" is the name of the new metric type, including quotation
marks, with each word capitalized, with words separated by spaces.

"""

import sys
import re

class TemplatedFile:

  filename = ""
  template = ""
  data = {}

  def __init__(self, template_dir, template_filename, template_data={}):
    self.directory = template_dir
    self.filename = template_filename
    self.data = template_data
    self.template = ""
    self.read_template_file()

  def read_template_file(self):
    filename = "%s/%s" % (self.directory, self.filename)
    template_file = open(filename, 'r')
    assert template_file, "could not open file %s" % (filename)
    self.template = template_file.read()
    template_file.close()

  def set_data(self, new_template_data):
    assert isinstance(new_template_data, dict), "new template data is not a dict object"
    self.data = new_template_data

  def write_to_file(self, output_dir=""):
    assert output_dir != "", "must specify an output directory"
    output_filename = "%s/%s" % (output_dir, self.filename)
    output_file_contents = self.template
    for name,value in self.data.iteritems():
      filename_pattern = "__%s__" % (name)
      content_pattern = "\/\*\$%s\*\/" % (name)
      output_filename = re.sub(str(filename_pattern), str(value), str(output_filename))
      output_file_contents = re.sub(str(content_pattern), str(value), str(output_file_contents))
    output_file = open(output_filename, 'w')
    assert output_file, "could not open file %s for writing" % (output_filename)
    output_file.write(output_file_contents)
    output_file.close()


def main():
  template_directory = "."
  template_object_filename = "src/datametric___metric_type__.cpp"
  template_header_filename = "include/datametric___metric_type__.h"
  output_directory = "/home/thelusiv/code/vdrift-branches/driver-training"
  # load the data
  data = {}
  assert len(sys.argv) > 1, "not enough arguments"
  metric_type = sys.argv[1]
  data["metric_type"] = metric_type.lower().replace(" ", "_")
  data["metric_class"] = "%sMETRIC" % (metric_type.upper().replace(" ", ""))
  data["metric_include"] = '"datametric_%s.h"' % (metric_type.lower().replace(" ", "_"))
  data["metric_type_name"] = '"%s"' % (metric_type.replace(" ", ""))
  data["metric_output_var"] = '"Test"'
  # load the templates
  template_header = TemplatedFile(template_directory, template_header_filename, data)
  template_object = TemplatedFile(template_directory, template_object_filename, data)
  # write the files
  template_header.write_to_file(output_directory)
  template_object.write_to_file(output_directory)

if __name__ == "__main__":
  main()

