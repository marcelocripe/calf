/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2008 Thor Harald Johansen <thj@thj.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#include <calf/metadata.h>
#include <calf/modules.h>

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL

/// Filterclavier --- MIDI controlled filter
class filterclavier_audio_module: 
        public audio_module<filterclavier_metadata>, 
        public filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>, 
        public line_graph_iface
    {        
        const float min_gain;
        const float max_gain;
        
        int last_note;
        int last_velocity;
            
    public:    
        filterclavier_audio_module() 
            : 
                min_gain(1.0),
                max_gain(32.0),
                last_note(-1),
                last_velocity(-1) {}
        
        void params_changed()
        { 
            inertia_filter_module::inertia_cutoff.set_inertia(
                note_to_hz(last_note + *params[par_transpose], *params[par_detune]));
            
            float min_resonance = param_props[par_max_resonance].min;
             inertia_filter_module::inertia_resonance.set_inertia( 
                     (float(last_velocity) / 127.0)
                     // 0.001: see below
                     * (*params[par_max_resonance] - min_resonance + 0.001)
                     + min_resonance);
                 
            adjust_gain_according_to_filter_mode(last_velocity);
            
            inertia_filter_module::calculate_filter(); 
        }
            
        void activate()
        {
            inertia_filter_module::activate();
        }
        
        void set_sample_rate(uint32_t sr)
        {
            inertia_filter_module::set_sample_rate(sr);
        }

        
        void deactivate()
        {
            inertia_filter_module::deactivate();
        }
      
        /// MIDI control
        virtual void note_on(int note, int vel)
        {
            last_note     = note;
            last_velocity = vel;
            inertia_filter_module::inertia_cutoff.set_inertia(
                    note_to_hz(note + *params[par_transpose], *params[par_detune]));

            float min_resonance = param_props[par_max_resonance].min;
            inertia_filter_module::inertia_resonance.set_inertia( 
                    (float(vel) / 127.0) 
                    // 0.001: if the difference is equal to zero (which happens
                    // when the max_resonance knom is at minimum position
                    // then the filter gain doesnt seem to snap to zero on most note offs
                    * (*params[par_max_resonance] - min_resonance + 0.001) 
                    + min_resonance);
            
            adjust_gain_according_to_filter_mode(vel);
            
            inertia_filter_module::calculate_filter();
        }
        
        virtual void note_off(int note, int vel)
        {
            if (note == last_note) {
                inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
                inertia_filter_module::inertia_gain.set_inertia(min_gain);
                inertia_filter_module::calculate_filter();
                last_velocity = 0;
            }
        }

        bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
        bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
        
    private:
        void adjust_gain_according_to_filter_mode(int velocity) {
            int   mode = dsp::fastf2i_drm(*params[par_mode]);
            
            // for bandpasses: boost gain for velocities > 0
            if ( (mode_6db_bp <= mode) && (mode <= mode_18db_bp) ) {
                // gain for velocity 0:   1.0
                // gain for velocity 127: 32.0
                float mode_max_gain = max_gain;
                // max_gain is right for mode_6db_bp
                if (mode == mode_12db_bp)
                    mode_max_gain /= 6.0;
                if (mode == mode_18db_bp)
                    mode_max_gain /= 10.5;
                
                inertia_filter_module::inertia_gain.set_now(
                        (float(velocity) / 127.0) * (mode_max_gain - min_gain) + min_gain);
            } else {
                inertia_filter_module::inertia_gain.set_now(min_gain);
            }
        }
    };

#endif
    
};

#endif
