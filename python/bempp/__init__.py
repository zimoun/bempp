""" Boundary Element Method package BEM++ """

__all__=['grid_from_sphere',
         'grid_from_element_data',
         'structured_grid',
         'function_space',
         'GridFunction',
         'global_parameters',
         'BlockedBoundaryOperator',
         'export',
         'GridFactory',
         'import_grid',
         'FileReader']

# This imports dolfin at the same time as bempp if available to avoid delays
# at later imports of dolfin

try:
    import dolfin
except:
    have_dolfin = False
else:
    have_dolfin = True

from bempp.grid import Grid,grid_from_sphere,grid_from_element_data, structured_grid
from bempp.assembly import GridFunction
from bempp.assembly import BlockedBoundaryOperator
from bempp.space import function_space
from bempp.file_interfaces import FileReader, import_grid, export
from bempp.common import global_parameters
from bempp.grid import GridFactory

def test():
    """ Runs BEM++ python unit tests """
    from py.test import main
    from os.path import dirname
    main(dirname(__file__))
