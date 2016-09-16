from collections import OrderedDict

from bpy.path  import abspath
from bpy.types import Node
from bpy.props import BoolProperty, FloatProperty, FloatVectorProperty, StringProperty, EnumProperty

from ..data.materials import conductor_material_items
from ..nodes import (
    MitsubaNodeTypes, mitsuba_node
)


microfacet_distribution_items = [
    ('beckmann', 'Beckmann', 'beckmann'),
    ('ggx', 'Ggx', 'ggx'),
    ('phong', 'Phong', 'phong'),
]


class mitsuba_bsdf_node(mitsuba_node):
    bl_icon = 'MATERIAL'
    bl_width_min = 180
    shader_type_compat = {'OBJECT'}
    mitsuba_nodetype = 'BSDF'

    def draw_ior_menu_box(self, layout, attr, name, menu):
        box = layout.box()
        box.context_pointer_set("node", self)

        if attr == 'intIOR' and self.intIOR == self.intIOR_presetvalue:
            menu_text = self.intIOR_presetstring

        elif attr == 'extIOR' and self.extIOR == self.extIOR_presetvalue:
            menu_text = self.extIOR_presetstring

        elif attr == 'extEta' and self.extEta == self.extEta_presetvalue:
            menu_text = self.extEta_presetstring

        else:
            menu_text = '-- Choose %s preset --' % name

        box.menu('MITSUBA_MT_%s_ior_presets' % menu, text=menu_text)
        box.prop(self, attr)
        
@MitsubaNodeTypes.register
class MtsNodeBsdf_thunderloom(mitsuba_bsdf_node, Node):
    '''ThunderLoom material node'''
    bl_idname = 'MtsNodeBsdf_thunderloom'
    bl_label = 'thunderloom'
    plugin_types = {'thunderloom_mitsuba'}

    def draw_buttons(self, context, layout):
        layout.prop(self, 'wiffile')
        layout.prop(self, 'uscale')
        layout.prop(self, 'vscale')
        layout.prop(self, 'yrn0_specular_strength')
        layout.prop(self, 'yrn0_delta_x')
        layout.prop(self, 'yrn0_bend')
        layout.prop(self, 'yrn0_alpha')
        layout.prop(self, 'yrn0_beta')
        layout.prop(self, 'yrn0_psi')

    wiffile = StringProperty(name = 'WIF file', subtype = 'FILE_PATH')
    uscale = FloatProperty(name = 'U scale', default = 1.0)
    vscale = FloatProperty(name = 'V scale', default = 1.0)
    yrn0_specular_strength = FloatProperty(name = 'Specular Strength', default = 0.5)
    yrn0_delta_x  = FloatProperty(name = 'Highlight deltaX',    default = 0.3)
    yrn0_bend    = FloatProperty(name = 'U max', default = 0.7)
    yrn0_alpha   = FloatProperty(name = 'Alpha', default = 0.05)
    yrn0_beta    = FloatProperty(name = 'Beta', default = 2.0)
    yrn0_psi     = FloatProperty(name = 'Psi', default = 0.5)

    intensity_fineness = FloatProperty(name = 'Intensity var fineness', default = 0.0)

    custom_inputs = [
        {'type': 'MtsSocketColor_diffuseReflectance', 'name': 'Diffuse Reflectance'},
    ]

    custom_outputs = [
        {'type': 'MtsSocketBsdf', 'name': 'Bsdf'},
    ]

    def get_bsdf_dict(self, export_ctx):
        params = {
            'type': 'thunderloom_mitsuba',
            'wiffile': abspath(self.wiffile),
            'uscale': self.uscale,
            'vscale': self.vscale,
            'yrn0_specular_strength'   : self.yrn0_specular_strength,
            'yrn0_delta_x'   : self.yrn0_delta_x,
            'yrn0_bend'   : self.yrn0_bend,
            'yrn0_alpha'   : self.yrn0_alpha,
            'yrn0_beta'   : self.yrn0_beta,
            'yrn0_psi'   : self.yrn0_psi,
        }

        return params

    def set_from_dict(self, ntree, params):
        if 'reflectance' in params:
            self.inputs['Diffuse Reflectance'].set_color_socket(ntree, params['reflectance'])